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
from machine import PWM, Pin, Timer
#from rp2 import DMA
import _thread

sys.path.append("/Games/PeanutGB")
os.chdir("/Games/PeanutGB")

import PeanutGB

clockRate = 280 * 1000 * 1000

gc.collect()
# Allocate ram before rom read fragmentation occurs
romArray = None
maxCartRAMSize = 32 * 1024
saveSizeBuffer = array.array('i', [0])
saveFileSize = 0
ramData = bytearray(maxCartRAMSize)
scratchFilterBuffer = bytearray(128 * 144 * 2)
romFSBuffer = array.array('I', [0] * 3)
bankSize = 16 * 1024
numTableEntries = 4
# Resident banks
bankTable = bytearray(numTableEntries * bankSize)
gc.collect()

romScratchAddress = 0
scratchData = bytearray([])
romFile = None
fsCallback = None
romFileName = None
gameName = None
saveStateSelected = False

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

# Linear block allocator
class BlockAllocator:
    def __init__(self):
        # Leave ScratchPageOffset of pages for engine resources
        self.ScratchPageOffset = 64
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

maxResidentScratchRomSize = (1024 + 512) * 1024
def LoadROMScratch(fileName, maxRomPages = maxResidentScratchRomSize):
    global romScratchAddress
    global romFile
    loadingText = TextSprite("")
    GlobalBlockAllocator.Reset()
    totalDataRead = 0
    statData = os.stat(fileName)
    romSize = statData[6]
    maxScratchSize = min(maxRomPages, maxResidentScratchRomSize)
    romSizeScratch = min(romSize, maxScratchSize)
    romFile = open(fileName, "rb")
    romScratchAddress = GlobalBlockAllocator.GetBaseAddress()
    while totalDataRead < romSizeScratch:
        percentStr = str(int(totalDataRead / romSizeScratch * 100.0 + 1.0))
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

machine.freq(clockRate)

controllerStateBuffer = array.array('B', [0xff])

def InitEmulator():
    global romFileName
    global romArray
    global romFSBuffer
    global bankTable
    global saveFileSize
    romFileName = gameName + ".gb"
    # Load enough pages to initialize memory
    LoadROMScratch(romFileName, 4096 * 4)
    romArray = romScratchAddress
    PeanutGB.InitPeanut(
        romArray,
        saveSizeBuffer,
        romFSBuffer,
        bankTable
        )
    saveFileSize = saveSizeBuffer[0]

CART_SAVE_IDX = 0
AUTO_CART_SAVE_IDX = 1
SAVE_STATE_0_IDX = 2
SAVE_STATE_1_IDX = 3
SAVE_STATE_2_IDX = 4

CART_SAVE_STR = "Cart Save"
AUTO_CART_STR = "Cart Save State"
SAVE_0_STR = "Save State 0"
SAVE_1_STR = "Save State 1"
SAVE_2_STR = "Save State 2"
SAVE_STR_LIST = [CART_SAVE_STR, AUTO_CART_STR, SAVE_0_STR, SAVE_1_STR, SAVE_2_STR]

CART_SUFFIX = ".sav"
AUTO_CART_SUFFIX = "_auto.sav"
SAVE_0_SUFFIX = "_0.sav"
SAVE_1_SUFFIX = "_1.sav"
SAVE_2_SUFFIX = "_2.sav"
SAVE_SUFFIX_LIST = [CART_SUFFIX, AUTO_CART_SUFFIX, SAVE_0_SUFFIX, SAVE_1_SUFFIX, SAVE_2_SUFFIX]

def PopulateSaveList(romName):
    # Cart Save, Auto Cart State, Save State 0, Save State 1, Save State 2
    saveList = [None, None, None, None, None]
    saveFiles = []
    allFiles = os.listdir()
    for file in allFiles:
        if file.startswith(romName) and file.endswith(".sav"):
            saveFiles.append(file)
    for file in saveFiles:
        if file == romName + CART_SUFFIX:
            saveList[CART_SAVE_IDX] = file
        elif file.endswith(AUTO_CART_SUFFIX):
            saveList[AUTO_CART_SAVE_IDX] = file
        elif file.endswith(SAVE_0_SUFFIX):
            saveList[SAVE_STATE_0_IDX] = file
        elif file.endswith(SAVE_1_SUFFIX):
            saveList[SAVE_STATE_1_IDX] = file
        elif file.endswith(SAVE_2_SUFFIX):
            saveList[SAVE_STATE_2_IDX] = file
    return saveList

def SaveLoadMenu(romName, bSaveMode):
    global camera
    bFileSelected = False
    saveList = PopulateSaveList(romName)
    
    if (not(bSaveMode) and len(saveList) <= 0):
        InitEmulator()
        return True
    
    selectedColor = Color(1.0, 1.0, 1.0)
    unselectedColor = Color(0.5, 0.5, 0.5)
    
    cameraPositionY = -1000
    camera.position = Vector3(8, cameraPositionY, 0)
    
    titleText = TextSprite("Select Save")
    titleText.position = Vector3(0, cameraPositionY + -4 * positionSpacing, 0)
    saveTextList = []
    saveTextListInd = []
    saveFileNameList = []
    
    saveIndex = 0
    if (bSaveMode):
        for save in saveList:
            # Skip auto cart save
            if (saveIndex != AUTO_CART_SAVE_IDX) and not(saveIndex == CART_SAVE_IDX and saveFileSize <= 0):
                saveTextNode = None
                if (save == None):
                    saveTextNode = TextSprite("Empty " + SAVE_STR_LIST[saveIndex])
                else:
                    saveTextNode = TextSprite(SAVE_STR_LIST[saveIndex])
                saveTextNode.position = Vector3(0, cameraPositionY + (saveIndex - 2) * positionSpacing, 0)
                saveTextList.append(saveTextNode)
                saveTextListInd.append(saveIndex) 
            saveIndex += 1
                
    else: # Load Mode
        for save in saveList:
            saveTextNode = None
            if saveIndex == CART_SAVE_IDX and saveFileSize <= 0:
                saveTextNode = TextSprite("Continue")
                saveTextNode.position = Vector3(0, cameraPositionY + (saveIndex - 2) * positionSpacing, 0)
                saveTextList.append(saveTextNode)
                saveTextListInd.append(saveIndex)
            elif saveIndex == AUTO_CART_SAVE_IDX and saveFileSize <= 0:
                pass
            elif (save != None):
                saveTextNode = TextSprite(SAVE_STR_LIST[saveIndex])
                saveTextNode.position = Vector3(0, cameraPositionY + (saveIndex - 2) * positionSpacing, 0)
                saveTextList.append(saveTextNode)
                saveTextListInd.append(saveIndex)
            saveIndex += 1
    
    for i in range(len(SAVE_SUFFIX_LIST)):
        saveFileNameList.append(romName + SAVE_SUFFIX_LIST[i])
    
    numSaves = len(saveTextListInd)
    
    timer = GameTimer()
    inputDelayTime = 0.3
    timer.Reset(inputDelayTime)
    lastTime = time.ticks_ms()
    
    selectedSave = 0
    while (True):
        engine.tick()
        
        currentTime = time.ticks_ms()
        timer.Tick((currentTime - lastTime) / 1000.0)
        lastTime = currentTime
        
        if (engine_io.UP.is_pressed and timer.IsDone()):
            timer.Reset(inputDelayTime)
            selectedSave -= 1
        if (engine_io.DOWN.is_pressed and timer.IsDone()):
            timer.Reset(inputDelayTime)
            selectedSave += 1

        if (selectedSave < 0):
            selectedSave = numSaves - 1
        if (selectedSave >= numSaves):
            selectedSave = 0
            
        for i in range(len(saveTextList)):
            if i == selectedSave:
                saveTextList[i].SetColor(selectedColor)
            else:
                saveTextList[i].SetColor(unselectedColor)
            
        if (engine_io.A.is_pressed and timer.IsDone()):
            saveEnum = saveTextListInd[selectedSave]
            if (bSaveMode):
                if (saveEnum == CART_SAVE_IDX): # Cart Save
                    SaveCart(saveFileNameList[CART_SAVE_IDX], saveFileNameList[AUTO_CART_SAVE_IDX])
                else: # Save State
                    SaveState(saveFileNameList[saveEnum])
            else: # Load Mode
                if (saveEnum == CART_SAVE_IDX): # Cart Load
                    if (saveFileSize <= 0): # Continue no cart ram option
                        return True
                    LoadCart(saveFileNameList[CART_SAVE_IDX])
                else: # Load State
                    LoadState(saveFileNameList[saveEnum])
            bFileSelected = True
            break
        if (engine_io.B.is_pressed and timer.IsDone()):
            # Exit out to previous state
            bFileSelected = False
            break;
        
    camera.position = Vector3(8, 0, 0)
    return bFileSelected
        
def SaveMessageMenu(saveFileName):
    global saveFileSize
    saveWaitGameTimer = GameTimer()
    lastTime = time.ticks_ms()
    print("On cart Ram is " + str(saveFileSize) + " bytes.")
    
    savingText = TextSprite("Saving to")
    savingText.position = Vector3(0, -1 * positionSpacing, 0)
    savingText2 = TextSprite(saveFileName)
    savingText2.position = Vector3(0, 0 * positionSpacing, 0)
    savingText3 = TextSprite("Remember to")
    savingText3.position = Vector3(0, 2 * positionSpacing, 0)
    savingText4 = TextSprite("save in game.")
    savingText4.position = Vector3(0, 3 * positionSpacing, 0)
    
    while (True):
        engine.tick()
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        saveWaitGameTimer.Tick(deltaTimeS)
        if (saveWaitGameTimer.IsDone()):
            break
    gc.collect()

def LoadCart(fileName):
    global ramData
    ramDataMemView = memoryview(ramData)
    fileH = open(fileName, 'rb')
    fileH.readinto(ramDataMemView)
    fileH.close()
    
def LoadState(fileName):
    global ramData
    ramDataMemView = memoryview(ramData)
    fileH = open(fileName, 'rb')
    fileH.seek(maxCartRAMSize, 0)
    fileH.readinto(ramDataMemView)
    PeanutGB.SetSaveState(ramData)
    fileH.seek(0, 0)
    fileH.readinto(ramDataMemView)
    fileH.close()

def SaveCart(fileName, fileNameAutoSaveState):
    global ramData
    fileH = open(fileName, 'wb')
    fileH.write(ramData)
    fileH.close()
    # Auto save state for each cart save
    SaveState(fileNameAutoSaveState)
    SaveMessageMenu(fileName)
    
def SaveState(fileName):
    global ramData
    fileH = open(fileName, 'wb')
    fileH.write(ramData)
    PeanutGB.FillSaveState(ramData)
    fileH.write(ramData)
    fileH.close()
    # Reset RAM data to continue running
    fileH = open(fileName, 'rb')
    fileH.readinto(ramData)
    fileH.close()
    SaveMessageMenu(fileName)

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

class GameTimer():
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
    
    timer = GameTimer()
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
            InitEmulator()
            bSaveSelected = SaveLoadMenu(gameName, False)
            if (bSaveSelected):
                camera.position = Vector3(8, 0, 0)
                break
        
    for text in textList:
        text.mark_destroy_all()
    titleText.mark_destroy_all()
    titleText2.mark_destroy_all()
    titleText3.mark_destroy_all()
    titleText4.mark_destroy_all()
    headerText.mark_destroy_all()
    
    waitGameTimer = GameTimer()
    waitTime = 0.3
    waitGameTimer.Reset(waitTime)
    lastTime = time.ticks_ms()
    
    selectedRom = 0
    while (True):
        engine.tick()
        gc.collect()
        
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        waitGameTimer.Tick(deltaTimeS)
        
        if (waitGameTimer.IsDone()):
            break;
        
def PanningMenu():
    global positionSpacing
    engine.tick()
    gc.collect()
    # Tool tip for no scaling mode
    toolTipTime = 1.5
    toolTipGameTimer = GameTimer()
    toolTipGameTimer.Reset(toolTipTime)
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
        toolTipGameTimer.Tick(deltaTimeS)
        if (toolTipGameTimer.IsDone() and (engine_io.A.is_pressed or engine_io.B.is_pressed or engine_io.MENU.is_pressed or engine_io.RB.is_pressed)):
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
    inputGameTimer = GameTimer()
    inputGameTimer.Reset(inputTime)
    
    lastTime = time.ticks_ms()
    
    while (True):
        engine.tick()
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        inputGameTimer.Tick(deltaTimeS)
    
        if (inputGameTimer.IsDone() and
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
    
    inputGameTimer = GameTimer()
    leaveMenuRBGameTimer = GameTimer()
    inputDelayTime = 0.3
    leaveMenuRBTime = 2.0
    inputGameTimer.Reset(inputDelayTime)
    leaveMenuRBGameTimer.Reset(leaveMenuRBTime)
    lastTime = time.ticks_ms()

    while(True):
        engine.tick()
        
        currentTime = time.ticks_ms()
        deltaTimeS = (currentTime - lastTime) / 1000.0
        lastTime = currentTime
        
        inputGameTimer.Tick(deltaTimeS)
        leaveMenuRBGameTimer.Tick(deltaTimeS)
        
        menuText.color = unselectedColor
        menuText2.color = unselectedColor
        menuText3.color = unselectedColor
        menuText4.color = unselectedColor
        menuText5.color = unselectedColor
        menuText6.color = unselectedColor
        menuText7.color = unselectedColor
        menuText8.color = unselectedColor

        if (inputGameTimer.IsDone() and (engine_io.UP.is_pressed or engine_io.DOWN.is_pressed)):
            inputGameTimer.Reset(inputDelayTime)
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
        
        if (leaveMenuRBGameTimer.IsDone() and engine_io.RB.is_pressed):
            break
        
        if (menuSelection == 0): # Save
            menuText6.color = selectedColor
            if (inputGameTimer.IsDone() and engine_io.A.is_pressed):
                bFileSelected = SaveLoadMenu(gameName, True)
                inputGameTimer.Reset(inputDelayTime)
                if (bFileSelected):
                    break
        elif (menuSelection == 1): # brightness
            menuText7.color = selectedColor
            if (inputGameTimer.IsDone() and (engine_io.LEFT.is_pressed or engine_io.RIGHT.is_pressed or engine_io.A.is_pressed)):
                inputGameTimer.Reset(inputDelayTime)
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
            if (inputGameTimer.IsDone() and (engine_io.LEFT.is_pressed or engine_io.RIGHT.is_pressed or engine_io.A.is_pressed)):
                inputGameTimer.Reset(inputDelayTime)
                scalingType += 1
                if (scalingType > 1):
                    scalingType = 0
            if (scalingType == 0):
                menuText2.text = windowScalingText
            else:
                menuText2.text = noScalingText
        elif (menuSelection == 4): # game speed
            menuText3.color = selectedColor
            if (inputGameTimer.IsDone() and (engine_io.LEFT.is_pressed or engine_io.RIGHT.is_pressed or engine_io.A.is_pressed)):
                inputGameTimer.Reset(inputDelayTime)
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
            if (inputGameTimer.IsDone() and engine_io.A.is_pressed):
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

from machine import mem32

audioEnablePin = None
pwmPin = None
audioSamples = None
dmaSamples = None
maxSamples = 0
audioSampleIndex = 0
audioDMA = None
pwmCCReg = None
audioDMAControl = None
audioFQ = 100 * 1000
pwmTOP = 0
sampleIndex = 0
numBuffers = 8
currentBuffer = 0

def InitAudio():
    global audioDMA
    global audioEnablePin
    global pwmPin
    global audioSamples
    global dmaSamples
    global maxSamples
    global pwmCCReg
    global audioDMAControl
    global audioFQ
    global pwmTOP
    
    print("PWM FQ " + str(audioFQ))
    numSamplesBuffer = array.array('i', [0])
    # Audio Init
    PeanutGB.InitAPU(numSamplesBuffer)
    
    audioEnablePinNum = 20
    pwmPinNum = 23
    audioEnablePin = Pin(audioEnablePinNum, Pin.OUT)
    audioEnablePin.value(0)
    pwmPin = PWM(Pin(pwmPinNum))
    pwmPin.freq(audioFQ)
        
    pwmSlice = 3
    pwmChannel = 1 # B Channel
    
    print("slice " + str(pwmSlice))
    print("channel " + str(pwmChannel))
    
    # PWM CC register address
    pwmRegBase = 0x400a8000
    offsetToCCReg = 0x00c
    registersPerChannel = 0x14
    pwmCCReg = pwmRegBase + pwmSlice * registersPerChannel + offsetToCCReg + pwmChannel * 2
    print("pwmCCReg " + hex(pwmCCReg))
    pwmTOP = mem32[pwmCCReg + 2]
    print("pwmTOPReg " + str(pwmTOP))
    
    audioSamples = array.array('h', [0] * 4)
    dmaSamples = array.array('H', [0] * 4)
    
    print("DMA Samples " + str(len(dmaSamples)))
    
    audioEnablePin.value(1)
    
    pwmPin.duty_u16(0)

    return

    # Setup PWM before DMA
    DREQ_PWM_WRAP0 = 32

    audioDMA = DMA()
    print("DMA channel " + str(audioDMA.channel))

    audioDMAControl = audioDMA.pack_ctrl(
        size          = 1,    # ushort writes
        inc_read      = True,
        inc_write     = False,
        treq_sel      = DREQ_PWM_WRAP0 + pwmSlice,
        high_pri      = True
    )

    audioDMA.config(
        read      = dmaSamples,
        write     = pwmCCReg,
        count     = len(dmaSamples),
        ctrl      = audioDMAControl,
        trigger   = True
    )

sampleRate = 8 * 1000
delay = int(1_000_000 / sampleRate)

SAMPLE_RATE = 8000
FREQ = 440
phase1 = 0
phase2 = 0
AMP1 = 8000  * 4
AMP2 = 8000  * 4

#@micropython.native
def audio_tick():
    global pwmPin
    global pwmTOP
    global dmaSamples
    global AMP1
    global phase1
    global AMP2
    global phase2
    
    phase_inc1 = dmaSamples[0] / SAMPLE_RATE
    phase_inc2 = dmaSamples[2] / SAMPLE_RATE

    AMP1 = dmaSamples[1] * 2024
    AMP2 = dmaSamples[3] * 1024

    phase1 += phase_inc1
    if phase1 >= 1.0:
        phase1 -= 1.0
    
    phase2 += phase_inc2
    if phase2 >= 1.0:
        phase2 -= 1.0

    # square wave
    s = AMP1 if phase1 < 0.5 else -AMP1
    
    s2 = AMP2 if phase2 < 0.5 else -AMP2

    # center around 50% duty
    pwmPin.duty_u16(((32768 + s + s2) * pwmTOP) // 65535)

def AudioLoop():
    while (True):
        time.sleep_us(delay)
        audio_tick()
        
# Read from FS for ROM banks past 1.5MB
# ROM reads in this range happen on core1
# Natmod callbacks do not work with this firmware
# ISRs see frozen globals from Python VM
# This routine only needs to run when processing emulator steps
def FillTableEntry(tableID, bankID, romFileH, memView):
    global romFSFile
    global bankTable
    global bankSize
    romFileH.seek(bankID * bankSize, 0)
    romFileH.readinto(memView[tableID * bankSize : (tableID + 1) * bankSize])
def RomBankTableUpdate():
    global romFSBuffer
    global romFileH
    global romFileName
    romFileH = open(romFileName, 'rb')
    memView = memoryview(bankTable)
    while (True):
        if (romFSBuffer[0] == 0): # No ROM bank table miss
            continue
        else:
            bankID = romFSBuffer[1]
            tableID = romFSBuffer[2]
            FillTableEntry(tableID, bankID, romFileH, memView)
            romFSBuffer[0] = 0 # ROM bank table update finished

def Main():
    global romScratchAddress
    global gameName
    global romFileName
    global romSelected
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
    global audioSamples
    global dmaSamples
    global audioFQ
    global pwmTOP
    
    global currentBuffer
    global sampleRate
    
    global romFSBuffer
    global romRSTimer
    
    global romFSFile
        
    engine.tick()
    engine.fps_limit(30)
    
    RomSelectMenu()
    engine.tick()
    gc.collect()
    InfoMenu()
    
    LoadROMScratch(romFileName)
    
    _thread.start_new_thread(RomBankTableUpdate, ())
    
    print("Peanut Initialized")
    
    # On cart RAM can be used for tasks other than save data
    # must allocate in device RAM
    engine.tick()
    gc.collect()
    freeBytes = gc.mem_free()
    print("Total fragmented bytes free before cart RAM allocation: " + str(freeBytes))        
                
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
    displayPanGameTimer = GameTimer()
    menuGameTimer = GameTimer()
    
    bAPUEnabled = False
    bPPUEnabled = True
    bEnableFrameSkip = True
        
    freeBytes = gc.mem_free()
    print("Total bytes free after all RAM allocations: " + str(freeBytes))
    
    if (bAPUEnabled):
        InitAudio()
        #_thread.start_new_thread(AudioLoop, ())
    gc.collect()
    
    while(True):
        if engine.tick():
            if (menuGameTimer.IsDone() and engine_io.RB.is_pressed):
                menuGameTimer.Reset(menuTime)
                # Handle quick save menu
                if (engine_io.MENU.is_pressed):
                    SaveLoadMenu(gameName, True)
                else:
                    SettingsMenu()
                    # Move time ticks up to current time
                    lastTime = time.ticks_ms()
                    if (menuShowControls):
                        menuShowControls = False
                        InfoMenu()
                    if (menuPanning):
                        menuPanning = False
                        PanningMenu()
                    gc.collect()
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
            
            displayPanGameTimer.Tick(deltaTimeS)
            menuGameTimer.Tick(deltaTimeS)
                    
            if (displayPanGameTimer.IsDone() and engine_io.LB.is_pressed and not engine_io.MENU.is_pressed):
                if (engine_io.LEFT.is_pressed):
                    noScaleOffsetX -= 1
                    displayPanGameTimer.Reset(displayPanTime)
                if (engine_io.RIGHT.is_pressed):
                    noScaleOffsetX += 1
                    displayPanGameTimer.Reset(displayPanTime)
                if (engine_io.UP.is_pressed):
                    noScaleOffsetY -= 1
                    displayPanGameTimer.Reset(displayPanTime)
                if (engine_io.DOWN.is_pressed):
                    noScaleOffsetY += 1
                    displayPanGameTimer.Reset(displayPanTime)
                noScaleOffsetX = max(0, min(noScaleOffsetX, maxPanX))
                noScaleOffsetY = max(0, min(noScaleOffsetY, maxPanY))
            
            framesPerEmuSpeed = 1
            if (scalingType == 0):
                framesPerEmuSpeed = 2
            emulationFramesPerDeviceFrame = emuSpeed * framesPerEmuSpeed
            if not(bEnableFrameSkip):
                emulationFramesPerDeviceFrame = 1
            for emuFrame in range(emulationFramesPerDeviceFrame):
                PeanutGB.PeanutRun(
                    romArray,
                    controllerStateBuffer,
                    ramData
                    )

            if(bPPUEnabled):
                UpdateSettingsBuffer()
                displayBuffer = engine_draw.back_fb_data()
                PeanutGB.ResampleBuffer(
                    displayBuffer,
                    scratchFilterBuffer,
                    displaySettingsBuffer
                    )
            
            if(bAPUEnabled):
                samplingSettings = array.array('i', [0] * 3)
                samplingSettings[0] = int(pwmTOP)
                samplingSettings[1] = 0
                PeanutGB.SampleAPU(
                    audioSamples,
                    dmaSamples,
                    samplingSettings)

Main()
    

