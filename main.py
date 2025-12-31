import os
import sys
import machine
import engine_main
import engine
import engine_io
import engine_draw
import array
import time
import gc
import errno
from engine_draw import Color
from engine_nodes import Text2DNode, CameraNode
from engine_math import Vector2, Vector3
from engine_resources import FontResource

sys.path.append("/Games/PeanutGB")
os.chdir("/Games/PeanutGB")

import PeanutGB

romScratchAddress = 0
scratchData = bytearray([])

camera = CameraNode()
textFont = FontResource("font3x5.bmp")
positionSpacing = 12
class TextSprite(Text2DNode):
    def __init__(self, text):
        global textFont
        super().__init__(self)
        self.text = text
        self.layer = 7
        self.position = Vector2(0, 0)
        self.font = textFont
        self.opacity = 1.0
        self.scale.x = 2
        self.scale.y = 2
        self.color = Color(1.0, 1.0, 1.0)
        self.letter_spacing = 0.5
    def SetColor(self, color):
        self.color = color
    def SetPosition(self, position):
        self.position = position

loadingText = TextSprite("")

# Linear block allocator
class BlockAllocator:
    def __init__(self):
        # Leave ScratchPageOffset of pages for engine resources
        self.ScratchPageOffset = 32
        self.ProgramSize = 1024*1024
        self.PageSize = 4096
        self.ScratchSize = 2*1024*1024
        # Two MB backwards from the start of fs memory
        self.ScratchMemoryPageStartAddress = -2 * self.ProgramSize//self.PageSize
        self.FreePageList = [range(self.ScratchSize//self.PageSize)]
        self.__NextPageAddress = self.ScratchPageOffset
        
    def Reset(self):
        self.__NextPageAddress = self.ScratchPageOffset
        
    # Test function to verify scratch memory location on device
    def ReadWriteFloatArrayTest(self, TestFloatArray):
        numfloats = len(TestFloatArray)
        PageAddressStart = self.ScratchMemoryPageStartAddress
        rp2.Flash().writeblocks(PageAddressStart, TestFloatArray)
        OutFloatArray = array.array('f', [0.0, 0.0, 0.0, 0.0])
        rp2.Flash().readblocks(PageAddressStart, OutFloatArray)
        print(OutFloatArray)
        
    def GetBaseAddress(self):
        return (self.ScratchMemoryPageStartAddress + 512 + 256 + self.__NextPageAddress) * 4096 + 0x10000000
        
    def WriteScratch(self, Data : bytearray):
        PageAddressStart = self.ScratchMemoryPageStartAddress + self.__NextPageAddress
        rp2.Flash().writeblocks(PageAddressStart, Data)
        ByteAddress = (self.ScratchMemoryPageStartAddress + 512 + 256 + self.__NextPageAddress) * 4096 + 0x10000000
        RoundPage = 0
        if ((len(Data) - ((len(Data) // 4096) * 4096)) > 0):
            RoundPage = 1
        self.__NextPageAddress = self.__NextPageAddress + (len(Data) // 4096) + RoundPage
        return ByteAddress
    
GlobalBlockAllocator = BlockAllocator()

def CheckFlush():
    global scratchData
    if(len(scratchData) == 4096):
        GlobalBlockAllocator.WriteScratch(scratchData)
        scratchData = bytearray([])
        gc.collect()

def ForceFlush():
    global scratchData
    if(len(scratchData) > 0):
        GlobalBlockAllocator.WriteScratch(scratchData)
        scratchData = bytearray([])
        gc.collect()

def LoadROMScratch(fileName):
    global romScratchAddress
    global loadingText
    totalDataRead = 0
    statData = os.stat(fileName)
    romSize = statData[6]
    romFile = open(fileName, "rb")
    romScratchAddress = GlobalBlockAllocator.GetBaseAddress()
    while totalDataRead < romSize:
        percentStr = str(int(totalDataRead / romSize * 100.0 + 1.0))
        loadingText.text = "Loading " + percentStr + " %"
        loadingProgress = f"Loading ROM " + percentStr + " percent."
        print(loadingProgress, end='\r')
        engine.tick()
        scratchData.extend(romFile.read(4096))
        totalDataRead += 4096
        CheckFlush()
    loadingText = None
    ForceFlush()
    romFile.close()
    gc.collect()

machine.freq(280 * 1000 * 1000)

controllerStateBuffer = array.array('B', [0xff])

def FileExists(fileName):
    bFileExists = False
    try:
        file = open(fileName, 'rb')
        bFileExists = True
        file.close()
    except OSError as e:
        if e.errno == errno.ENOENT:
            bFileExists = False
    return bFileExists

def SetControllerState(buttonIndex, bPressed):
    global controllerStateBuffer
    if (bPressed):
        controllerStateBuffer[0] &= ~buttonIndex
    else:
        controllerStateBuffer[0] |= buttonIndex

class Timer():
    def __init__(self):
        self.currentTime = 0.0
    def Tick(self, dt):
        self.currentTime -= dt
        if self.currentTime <= 0.0:
            self.currentTime = 0.0
    def Reset(self, time):
        self.currentTime = time
    def IsDone(self):
        return self.currentTime <= 0.0

gameName = ""
def RomSelectMenu():
    global gameName
    global positionSpacing
    # Find files ending in .gb
    gbFiles = []
    allFiles = os.listdir()
    for file in allFiles:
        if file.endswith(".gb"):
            gbFiles.append(file)
    numRoms = len(gbFiles)

    titleText = TextSprite("Peanut GB Emulator")
    titleText.position = Vector3(0, -4 * positionSpacing, 0)
    titleText2 = TextSprite("by Delta Beard")
    titleText2.position = Vector3(0, -3 * positionSpacing, 0)
    titleText3 = TextSprite("Ported to Thumby")
    titleText3.position = Vector3(0, -2 * positionSpacing, 0)
    titleText4 = TextSprite("by anonymous")
    titleText4.position = Vector3(0, -1 * positionSpacing, 0)

    headerText = None
    if (numRoms > 0):
        headerText = TextSprite("Select a ROM file.")
    else:
        headerText = TextSprite("No ROMs found.")
        headerText.position = Vector3(0, 1 * positionSpacing, 0)
        headerText2 = TextSprite("Place ROM files")
        headerText2.position = Vector3(0, 2 * positionSpacing, 0)
        headerText3 = TextSprite("in the PeanutGB")
        headerText3.position = Vector3(0, 3 * positionSpacing, 0)
        headerText4 = TextSprite("directory.")
        headerText4.position = Vector3(0, 4 * positionSpacing, 0)
        while (True):
            engine.tick()
            if (engine_io.A.is_pressed or engine_io.MENU.is_pressed):
                sys.exit()
    headerText.SetColor(Color(0.5, 0.0, 0.5))
    headerText.position = Vector3(0, 2 * positionSpacing, 0)
    
    positionY = 4 * positionSpacing
    selectedColor = Color(1.0, 1.0, 1.0)
    unselectedColor = Color(0.5, 0.5, 0.5)
    textList = []
    for fileName in gbFiles:
        textNode = TextSprite(fileName)
        textNode.SetPosition(Vector2(0, positionY))
        textNode.SetColor(unselectedColor)
        textList.append(textNode)
        positionY += positionSpacing
    
    timer = Timer()
    inputDelayTime = 0.3
    timer.Reset(inputDelayTime)
    lastTime = time.ticks_ms()
    
    selectedRom = 0
    while (True):
        engine.tick()
        if(numRoms <= 0):
            continue
        
        currentTime = time.ticks_ms()
        timer.Tick((currentTime - lastTime) / 1000.0)
        lastTime = currentTime
        
        cameraPositionY = selectedRom * positionSpacing + positionSpacing
        camera.position = Vector3(8, cameraPositionY, 0)
        if (engine_io.UP.is_pressed and timer.IsDone()):
            timer.Reset(inputDelayTime)
            selectedRom -= 1
        if (engine_io.DOWN.is_pressed and timer.IsDone()):
            timer.Reset(inputDelayTime)
            selectedRom += 1

        if (selectedRom < 0):
            selectedRom = numRoms - 1
        if (selectedRom >= numRoms):
            selectedRom = 0
            
        for i in range(len(textList)):
            if i == selectedRom:
                textList[i].SetColor(selectedColor)
            else:
                textList[i].SetColor(unselectedColor)
            
        if (engine_io.A.is_pressed):
            fileName = gbFiles[selectedRom]
            gameName = fileName[:-3]
            camera.position = Vector3(8, 0, 0)
            break
        
    for text in textList:
        text.mark_destroy_all()
    titleText.mark_destroy_all()
    titleText2.mark_destroy_all()
    titleText3.mark_destroy_all()
    titleText4.mark_destroy_all()
    headerText.mark_destroy_all()
    
    waitTimer = Timer()
    waitTime = 0.3
    waitTimer.Reset(waitTime)
    lastTime = time.ticks_ms()
    
    selectedRom = 0
    while (True):
        engine.tick()
        gc.collect()
        
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        waitTimer.Tick(deltaTimeS)
        
        if (waitTimer.IsDone()):
            break;
        
def PanningMenu():
    global positionSpacing
    engine.tick()
    gc.collect()
    # Tool tip for no scaling mode
    toolTipTime = 1.5
    toolTipTimer = Timer()
    toolTipTimer.Reset(toolTipTime)
    lastTime = time.ticks_ms()
    # Tool tip menu to expain features before returning to game
    noScaleText = TextSprite("LB + Direction")
    noScaleText.position = Vector3(0, -2 * positionSpacing, 0)
    noScaleText2 = TextSprite("to pan screen.")
    noScaleText2.position = Vector3(0, -1 * positionSpacing, 0)
    while (True):
        engine.tick()
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        toolTipTimer.Tick(deltaTimeS)
        if (toolTipTimer.IsDone() and (engine_io.A.is_pressed or engine_io.B.is_pressed or engine_io.MENU.is_pressed or engine_io.RB.is_pressed)):
            break
        
def InfoMenu():
    global positionSpacing
    engine.tick()
    gc.collect()
    infoText = TextSprite("D-Pad <> D-Pad")
    infoText.position = Vector3(0, -4 * positionSpacing, 0)
    infoText2 = TextSprite("A B <> A B")
    infoText2.position = Vector3(0, -3 * positionSpacing, 0)
    infoText3 = TextSprite("Menu <> Start")
    infoText3.position = Vector3(0, -2 * positionSpacing, 0)
    infoText4 = TextSprite("LB <> Select")
    infoText4.position = Vector3(0, -1 * positionSpacing, 0)
    infoText5 = TextSprite("RB <> EMU Menu")
    infoText5.position = Vector3(0, 0 * positionSpacing, 0)
    infoText6 = TextSprite("LB + Menu <> Save")
    infoText6.position = Vector3(0, 1 * positionSpacing, 0)
    infoText7 = TextSprite("LB + D-Pad <> Pan")
    infoText7.position = Vector3(0, 2 * positionSpacing, 0)
    
    infoText8 = TextSprite("Press to Continue")
    infoText8.position = Vector3(0, 4 * positionSpacing, 0)
    
    inputTime = 0.5
    inputTimer = Timer()
    inputTimer.Reset(inputTime)
    
    lastTime = time.ticks_ms()
    
    while (True):
        engine.tick()
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        inputTimer.Tick(deltaTimeS)
    
        if (inputTimer.IsDone() and
            (engine_io.A.is_pressed or
             engine_io.B.is_pressed or
             engine_io.MENU.is_pressed)):
            break

def GetFormatedEmuStr(speed):
    equivalentSpeedPercent = speed / 2.0
    return "{:.1f}".format(equivalentSpeedPercent)

def GetFormatedBrightnessStr(brightness):
    return "{:.0f}".format(brightness * 100.0)

scalingType = 0
maxPanX = 160 - 128
maxPanY = 144 - 128
noScaleOffsetX = maxPanX // 2 - 1
noScaleOffsetY = maxPanY // 2 - 1
emuSpeed = 2 # Default two emu frames per draw frame
maxEmuSpeed = 8
minEmuSpeed = 1
menuShowControls = False
menuSave = False
menuPanning = False
def SettingsMenu():
    global positionSpacing
    global scalingType
    global emuSpeed
    global maxEmuSpeed
    global menuShowControls
    global menuSave
    global menuPanning
    
    engine.tick()
    gc.collect()
    
    menuSelection = 0
    windowScalingText = "Window Scaling"
    noScalingText = "No Scaling"
    selectedColor = Color(1.0, 1.0, 1.0)
    unselectedColor = Color(0.5, 0.5, 0.5)
    secondSelectedColor = Color(1.0, 1.0, 0.0)
    unimplementedColor = Color(1.0, 0.0, 0.0)
    
    brightness = engine.setting_brightness()
            
    menuText6 = TextSprite("Save to Flash")
    menuText6.position = Vector3(0, -4 * positionSpacing, 0)
    menuText7 = TextSprite("")
    menuText7.position = Vector3(0, -3 * positionSpacing, 0)
    menuText7.text = "Brightness: " + GetFormatedBrightnessStr(brightness)
    menuText8 = TextSprite("Volume: X")
    menuText8.position = Vector3(0, -2 * positionSpacing, 0)
    menuText = TextSprite("Scaling:")
    menuText.position = Vector3(0, -1 * positionSpacing, 0)
    menuText2 = TextSprite("")
    menuText2.text = windowScalingText
    menuText2.position = Vector3(0, 0 * positionSpacing, 0)
    menuText3 = TextSprite("")
    menuText3.text = "Game Speed: " + GetFormatedEmuStr(emuSpeed)
    menuText3.position = Vector3(0, 1 * positionSpacing, 0)
    menuText5 = TextSprite("Controls")
    menuText5.position = Vector3(0, 2 * positionSpacing, 0)
    
    menuText4 = TextSprite("Quit Emulator")
    menuText4.position = Vector3(0, 4 * positionSpacing, 0)
    
    maxSelections = 7
    
    inputTimer = Timer()
    leaveMenuRBTimer = Timer()
    inputDelayTime = 0.3
    leaveMenuRBTime = 2.0
    inputTimer.Reset(inputDelayTime)
    leaveMenuRBTimer.Reset(leaveMenuRBTime)
    lastTime = time.ticks_ms()

    while(True):
        engine.tick()
        
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        
        inputTimer.Tick(deltaTimeS)
        leaveMenuRBTimer.Tick(deltaTimeS)
        
        menuText.color = unselectedColor
        menuText2.color = unselectedColor
        menuText3.color = unselectedColor
        menuText4.color = unselectedColor
        menuText5.color = unselectedColor
        menuText6.color = unselectedColor
        menuText7.color = unselectedColor
        menuText8.color = unselectedColor

        if (inputTimer.IsDone() and (engine_io.UP.is_pressed or engine_io.DOWN.is_pressed)):
            inputTimer.Reset(inputDelayTime)
            if (engine_io.UP.is_pressed):
                menuSelection -= 1
            if (engine_io.DOWN.is_pressed):
                menuSelection += 1
            if (menuSelection > maxSelections):
                menuSelection = 0
            if (menuSelection < 0):
                menuSelection = maxSelections
        
        if (engine_io.B.is_pressed or engine_io.MENU.is_pressed):
            break
        
        if (leaveMenuRBTimer.IsDone() and engine_io.RB.is_pressed):
            break
        
        if (menuSelection == 0): # Save
            menuText6.color = selectedColor
            if (inputTimer.IsDone() and engine_io.A.is_pressed):
                inputTimer.Reset(inputDelayTime)
                menuSave = True
                break
        elif (menuSelection == 1): # brightness
            menuText7.color = selectedColor
            if (inputTimer.IsDone() and (engine_io.LEFT.is_pressed or engine_io.RIGHT.is_pressed or engine_io.A.is_pressed)):
                inputTimer.Reset(inputDelayTime)
                brightnessChange = 0.0
                if (engine_io.LEFT.is_pressed):
                    brightnessChange = -0.05
                if (engine_io.RIGHT.is_pressed or engine_io.A.is_pressed):
                    brightnessChange = 0.05
                brightness = engine.setting_brightness()
                brightnessChange = brightness + brightnessChange
                brightnessChange = max(0.0, min(1.0, brightnessChange))
                engine.setting_brightness(brightnessChange, False)
                menuText7.text = "Brightness: " + GetFormatedBrightnessStr(brightnessChange)
        elif (menuSelection == 2): # volume
            menuText8.color = unimplementedColor
        elif (menuSelection == 3): # display type 
            menuText.color = selectedColor
            menuText2.color = secondSelectedColor
            if (inputTimer.IsDone() and (engine_io.LEFT.is_pressed or engine_io.RIGHT.is_pressed or engine_io.A.is_pressed)):
                inputTimer.Reset(inputDelayTime)
                scalingType += 1
                if (scalingType > 1):
                    scalingType = 0
            if (scalingType == 0):
                menuText2.text = windowScalingText
            else:
                menuText2.text = noScalingText
        elif (menuSelection == 4): # game speed
            menuText3.color = selectedColor
            if (inputTimer.IsDone() and (engine_io.LEFT.is_pressed or engine_io.RIGHT.is_pressed or engine_io.A.is_pressed)):
                inputTimer.Reset(inputDelayTime)
                if (engine_io.LEFT.is_pressed):
                    emuSpeed -= 1
                if (engine_io.RIGHT.is_pressed or engine_io.A.is_pressed):
                    emuSpeed += 1
                if (emuSpeed > maxEmuSpeed):
                    emuSpeed = minEmuSpeed
                if (emuSpeed < minEmuSpeed):
                    emuSpeed = maxEmuSpeed
                equivalentSpeedPercent = emuSpeed / 2.0
                menuText3.text = "Game Speed: " + GetFormatedEmuStr(emuSpeed)
        elif (menuSelection == 5):
            menuText5.color = selectedColor
            if (inputTimer.IsDone() and engine_io.A.is_pressed):
                menuShowControls = True
                break
        else: # quit emu, reboot kit
            menuText4.color = selectedColor
            if (engine_io.A.is_pressed):
                sys.exit()
    
    menuText.mark_destroy_all()
    menuText2.mark_destroy_all()
    menuText3.mark_destroy_all()
    menuText4.mark_destroy_all()
    menuText5.mark_destroy_all()
    menuText6.mark_destroy_all()
    menuText7.mark_destroy_all()
    menuText8.mark_destroy_all()
    gc.collect()
    engine.tick()

    if (scalingType == 1):
        menuPanning = True
          

displaySettingsBuffer = array.array('i', [0] * 3)
def UpdateSettingsBuffer():
    global displaySettingsBuffer
    global scalingType
    global noScaleOffsetX
    global noScaleOffsetY
    displaySettingsBuffer [0] = scalingType
    displaySettingsBuffer [1] = noScaleOffsetX
    displaySettingsBuffer [2] = noScaleOffsetY

def Main():
    global romScratchAddress
    global gameName
    global positionSpacing
    global displaySettingsBuffer
    global maxPanX
    global maxPanY
    global noScaleOffsetX
    global noScaleOffsetY
    global emuSpeed
    global scalingType
    global menuShowControls
    global menuSave
    global menuPanning
    engine.tick()
    engine.fps_limit(30)
    
    RomSelectMenu()
    engine.tick()
    gc.collect()
    InfoMenu()
    
    #gameName = "PokemonRed"
    romFileName = gameName + ".gb"
    #ROMs can exceed 256KB, allocate to scratch
    LoadROMScratch(romFileName)
    romArray = romScratchAddress
    saveSizeBuffer = array.array('i', [0])
            
    PeanutGB.InitPeanut(
        romArray,
        saveSizeBuffer
        )
    
    saveFileSize = saveSizeBuffer[0]
    print("On cart Ram is " + str(hex(saveFileSize)) + " bytes.")
    
    # On cart RAM can be used for tasks other than save data
    # must allocate in device RAM
    engine.tick()
    gc.collect()
    freeBytes = gc.mem_free()
    print("Total fragmented bytes free before cart RAM allocation: " + str(freeBytes))
    bSaving = False
    saveFile = None
    ramData = array.array('B', [])
    if(saveFileSize > 0):
        saveFileName = gameName + ".sav"
        bFileExists = FileExists(saveFileName)
        if (bFileExists):
            print("Save file " + saveFileName + " found.")
            saveFile = open(saveFileName, 'rb')
            totalRamRead = 0
            readSize = 256
            while totalRamRead < saveFileSize:
                readData = saveFile.read(readSize)
                tempReadArray = array.array('B', readData)
                ramData.extend(tempReadArray)
                totalRamRead += readSize
                gc.collect()
        else:
            print("No save found, " + saveFileName + " created.")
            saveFile = open(saveFileName, 'wb')
            saveFile.write(ramData)
        saveFile.close()
                
    JOYPAD_A            = 0x01
    JOYPAD_B            = 0x02
    JOYPAD_SELECT       = 0x04
    JOYPAD_START        = 0x08
    JOYPAD_RIGHT        = 0x10
    JOYPAD_LEFT         = 0x20
    JOYPAD_UP           = 0x40
    JOYPAD_DOWN         = 0x80
    
    lastTime = time.ticks_ms()
    saveWaitTime = 5.0
    saveMsgTime = 3.0
    displayPanTime = 0.05
    menuTime = 1.5
    saveMsgTimer = Timer()
    saveWaitTimer = Timer()
    displayPanTimer = Timer()
    menuTimer = Timer()
    savingText = None
    savingText2 = None
    savingText3 = None
    savingText4 = None
    
    while(True):
        if engine.tick():
            if (menuTimer.IsDone() and saveMsgTimer.IsDone() and engine_io.RB.is_pressed):
                menuTimer.Reset(menuTime)
                SettingsMenu()
                # Move time ticks up to current time
                lastTime = time.ticks_ms()

                if (menuShowControls):
                    menuShowControls = False
                    InfoMenu()
                if (menuPanning):
                    menuPanning = False
                    PanningMenu()
                continue
            else:
                SetControllerState(JOYPAD_LEFT, engine_io.LEFT.is_pressed)
                SetControllerState(JOYPAD_RIGHT, engine_io.RIGHT.is_pressed)
                SetControllerState(JOYPAD_UP, engine_io.UP.is_pressed)
                SetControllerState(JOYPAD_DOWN, engine_io.DOWN.is_pressed)
                SetControllerState(JOYPAD_A, engine_io.A.is_pressed)
                SetControllerState(JOYPAD_B, engine_io.B.is_pressed)
                SetControllerState(JOYPAD_START, engine_io.MENU.is_pressed)
                SetControllerState(JOYPAD_SELECT, engine_io.LB.is_pressed)
            
            currentTime = time.ticks_ms()
            deltaTimeS = (currentTime - lastTime) / 1000.0
            lastTime = currentTime
            
            saveMsgTimer.Tick(deltaTimeS)
            saveWaitTimer.Tick(deltaTimeS)
            displayPanTimer.Tick(deltaTimeS)
            menuTimer.Tick(deltaTimeS)
            

            if (menuSave or (not(bSaving) and saveWaitTimer.IsDone() and engine_io.LB.is_pressed and engine_io.MENU.is_pressed)):
                menuSave = False
                bSaving = True
                saveMsgTimer.Reset(saveMsgTime)
                saveWaitTimer.Reset(saveWaitTime)
                if (saveFileSize > 0):
                    print("Saving to " + saveFileName)
                    saveFile = open(saveFileName, 'wb')
                    saveFile.write(ramData)
                    saveFile.close()
                    savingText = TextSprite("Saving to")
                    savingText.position = Vector3(0, -1 * positionSpacing, 0)
                    savingText2 = TextSprite(saveFileName)
                    savingText2.position = Vector3(0, 0 * positionSpacing, 0)
                    savingText3 = TextSprite("Remember to")
                    savingText3.position = Vector3(0, 2 * positionSpacing, 0)
                    savingText4 = TextSprite("save in game.")
                    savingText4.position = Vector3(0, 3 * positionSpacing, 0)
                else:
                    savingText = TextSprite("ROM has no ")
                    savingText.position = Vector3(0, -1 * positionSpacing, 0)
                    savingText2 = TextSprite("save function.")
                    savingText2.position = Vector3(0, 0 * positionSpacing, 0)
                
            else:
                bSaving = False
                    
            if (displayPanTimer.IsDone() and engine_io.LB.is_pressed and not engine_io.MENU.is_pressed):
                if (engine_io.LEFT.is_pressed):
                    noScaleOffsetX -= 1
                    displayPanTimer.Reset(displayPanTime)
                if (engine_io.RIGHT.is_pressed):
                    noScaleOffsetX += 1
                    displayPanTimer.Reset(displayPanTime)
                if (engine_io.UP.is_pressed):
                    noScaleOffsetY -= 1
                    displayPanTimer.Reset(displayPanTime)
                if (engine_io.DOWN.is_pressed):
                    noScaleOffsetY += 1
                    displayPanTimer.Reset(displayPanTime)
                noScaleOffsetX = max(0, min(noScaleOffsetX, maxPanX))
                noScaleOffsetY = max(0, min(noScaleOffsetY, maxPanY))
                
            if (saveMsgTimer.IsDone()):
                savingText = None
                savingText2 = None
                savingText3 = None
                savingText4 = None
                gc.collect()
            else:
                continue
            
            framesPerEmuSpeed = 1
            if (scalingType == 0):
                framesPerEmuSpeed = 2
            emulationFramesPerDeviceFrame = emuSpeed * framesPerEmuSpeed
            for emuFrame in range(emulationFramesPerDeviceFrame):
                PeanutGB.PeanutRun(
                    romArray,
                    controllerStateBuffer,
                    ramData
                    )

            UpdateSettingsBuffer()

            displayBuffer = engine_draw.back_fb_data()
            PeanutGB.ResampleBuffer(
                displayBuffer,
                displaySettingsBuffer
                )
    
Main()




