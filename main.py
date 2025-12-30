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

sys.path.append("/Games/ThumbyRaster")
os.chdir("/Games/ThumbyRaster")

import PeanutGB

romScratchAddress = 0
scratchData = bytearray([])


camera = CameraNode()
textFont = FontResource("font3x5.bmp")
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
        percentStr = str(int(totalDataRead / romSize * 100.0))
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
        
def ClearControllerState():
    global controllerStateBuffer
    controllerStateBuffer = 0xff

gameName = ""
def RomSelectMenu():
    global gameName
    # Find files ending in .gb
    gbFiles = []
    allFiles = os.listdir()
    for file in allFiles:
        if file.endswith(".gb"):
            gbFiles.append(file)
    numRoms = len(gbFiles)
    headerText = None
    if (numRoms > 0):
        headerText = TextSprite("Select a ROM file.")
    else:
        headerText = TextSprite("No ROMs found.\nPlace ROM files in the \nPeanutGB directory.")
    headerText.SetColor(Color(0.5, 0.0, 0.5))
    
    positionSpacing = 12
    positionY = positionSpacing
    selectedColor = Color(1.0, 1.0, 1.0)
    unselectedColor = Color(0.5, 0.5, 0.5)
    textList = []
    for fileName in gbFiles:
        textNode = TextSprite(fileName)
        textNode.SetPosition(Vector2(0, positionY))
        textNode.SetColor(unselectedColor)
        textList.append(textNode)
        positionY += positionSpacing
    
    upPressed = False
    downPressed = False
    selectedRom = 0
    while (True):
        engine.tick()
        if(numRoms <= 0):
            continue
        
        cameraPositionY = selectedRom * positionSpacing + 16
        camera.position = Vector3(0, cameraPositionY, 0)
        if (engine_io.UP.is_pressed and not(upPressed)):
            selectedRom += 1
            upPressed = True
        else:
            upPressed = False
        if (engine_io.DOWN.is_pressed and not(downPressed)):
            selectedRom -= 1
            downPressed = True
        else:
            downPressed = False

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
            camera.position = Vector3(0, 0, 0)
            break
    

def Menu():
    # menu input
    # menu render
    pass
    
    
    
def Main():
    global romScratchAddress
    global gameName
    engine.tick()
    engine.fps_limit(60)
    
    RomSelectMenu()
    gc.collect()
    
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
    gc.collect()
    bSaving = False
    saveFile = None
    ramData = array.array("B", [0])
    if(saveFileSize > 0):
        ramData = array.array("B", [0] * saveFileSize)
        saveFileName = gameName + ".sav"
        bFileExists = FileExists(saveFileName)
        if (bFileExists):
            print("Save file " + saveFileName + " found.")
            saveFile = open(saveFileName, 'rb')
            readData = saveFile.read()
            ramData = array.array("B", readData)
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
    
    while(True):
        if engine.tick():
            if (engine_io.RB.is_pressed):
                ClearControllerState()
                Menu()
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
            
            if (saveFileSize):
                if (not(bSaving) and engine_io.LB.is_pressed and engine_io.RB.is_pressed and engine_io.MENU.is_pressed):
                    print("Saving to " + saveFileName)
                    saveFile = open(saveFileName, 'wb')
                    saveFile.write(ramData)
                    saveFile.close()
                    bSaving = True
                else:
                    bSaving = False
            
            PeanutGB.PeanutRun(
                romArray,
                controllerStateBuffer,
                ramData
                )

            displayBuffer = engine_draw.back_fb_data()
            PeanutGB.ResampleBuffer(
                displayBuffer
                )
    
Main()




