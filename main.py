import Adafruit_GPIO.SPI as SPI
import Adafruit_SSD1306

from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont

import RPi.GPIO as GPIO

import subprocess  
import time
import threading
from enum import Enum
import os
'''
常数区
'''
# Raspberry Pi pin configuration:
RST = None     # on the PiOLED this pin isnt used
# Note the following are only used with SPI:
DC = 23
SPI_PORT = 0
SPI_DEVICE = 0
# Font Deafult Size
FONT_DEFAULT_SIZE = 12
VOLUME_STEP = 10
'''
对象功能区
'''

# 图层管理
class dispHandle:
    handleLabel = ""
    def __init__(self,label):
        self.handleLabel = label
class dispManger:
    handleList = []
    coverHandle = 0
    disp = 0
    width = 0
    height = 0
    padding = -2
    top = padding
    bottom = 0
    def __init__(self):
        self.disp = Adafruit_SSD1306.SSD1306_128_64(rst=RST)
        self.disp.begin()
        self.disp.clear()
        self.disp.display()
        self.width = self.disp.width
        self.height = self.disp.height
        self.bottom = self.height-self.padding

    def allocateHandle(self,label,cover):
        handle = dispHandle(label)
        if cover :
            self.coverHandle = handle
        return handle
    def refershPanel(self,handle,image):
        if handle == self.coverHandle :
            self.disp.image(image)
            self.disp.display()
            return True
        else:
            return False
    def delHandle(self,handle):
        self.handleList.remove(handle)
    def switchCover(self,handle):
        self.coverHandle = handle
    def isCover(self,handle):
        if self.coverHandle == handle:
            return True
        else:
            return False
        
# 播放列表浏览
# 按键管理
PressedPin = [5,6,13,19,26,12,20]
Channel_EXIT = PressedPin[0]
Channel_NEXT = PressedPin[1]
Channel_PRE = PressedPin[2]
Channel_ENTER = PressedPin[3]
Channel_VOLUP = PressedPin[4]
Channel_VOLDN = PressedPin[5]
Channel_MENU = PressedPin[6]

# 资源加载
class ListMusicType(Enum):
    LOCAL_MUSIC_MP3 = 0

class ListMusic:
    type = ListMusicType.LOCAL_MUSIC_MP3
    # type 
    # 0: Local music file(mp3 format)
    content = ""
    def __init__(self,type,content):
        self.type = type
        self.content = content
    def __init__(self,content):
        self.content = content
# 播放器主进程
class PlayerMain(threading.Thread):
    image = 0
    handle = 0
    draw = 0
    MusicDir = ""
    playList = []
    SupportedSuffix = ["mp3"]
    SupportedSuffix_T = tuple(SupportedSuffix)
    MplayerProcess = 0
    # 组件
    PlayMusic = 0
    Title = ""
    Paused = False
    TimePos = 0
    Volume = 100
    def __init__(self,musicDir):
        super().__init__()  
        self._stop_event = threading.Event()
        self.MusicDir = musicDir
        # 扫描文件夹下所有mp3文件
        self.playList = self.loadListFromDirSurface(musicDir)
        print(self.playList)
        self.image = Image.new('1', (_dm.width, _dm.height))
        self.draw = ImageDraw.Draw(self.image)
        self.handle = _dm.allocateHandle("mainPlayer",True)
        
    def refersh(self):
        self.draw.multiline_text((0,_dm.top),f"{self.Title:<20}",font=_font,fill=255)
        _dm.refershPanel(self.handle,self.image)
    def loadListFromDirSurface(self,dirpath):
        playList = []
        files = os.listdir(dirpath)
        for file in files:
            file_d = os.path.join(dirpath,file)
            if os.path.isdir(file_d) == False and file_d.endswith(self.SupportedSuffix_T):
                playList.append(ListMusic(file_d))
        return playList
    def loadListFromDirSub(self,dirpath):
        playList = []
        files = os.listdir(dirpath)
        for file in files:
            file_d = os.path.join(dirpath,file)
            if os.path.isdir(file_d):
                playList += self.loadListFromDirSurface(file_d)
            elif file_d.endswith(self.SupportedSuffix_T):
                playList.append(ListMusic(file_d))
        return playList
    def music_exit(self):
        with lock:
            self.stop()
    def music_pause(self):
        with lock:
            if self.Paused:
    #            mixer.music.unpause()
            else:
     #           mixer.music.pause()
            self.Paused = not self.Paused
    def music_next(self):
        with lock:
            if len(self.playList) == 0 : return
            self.PlayMusic += 1
            if self.PlayMusic >= len(self.playList):
                self.PlayMusic = 0
      #      mixer.music.load(self.playList[self.PlayMusic].content)
                
    def music_front(self):
        if len(self.playList) == 0 : return
        with lock:
            self.PlayMusic -= 1
            if self.PlayMusic < 0:
                self.PlayMusic = len(self.playList) - 1
       #     mixer.music.load(self.playList[self.PlayMusic].content)
    def music_voldn(self):
        with lock:
            self.Volume -= VOLUME_STEP
            if(self.Volume<0):   self.Volume = 0
        #    mixer.music.set_volume(self.Volume/100)
    def music_volup(self):
        with lock:
            self.Volume += VOLUME_STEP
            if(self.Volume>100):   self.Volume = 100
        #    mixer.music.set_volume(self.Volume/100)
    def run(self):
        while not self._stop_event.is_set():
            time.sleep(0.1)
    def stop(self):  
        self._stop_event.set()
        #mixer.music.unload()
    

KEY_CONTROL  = 1
player = 0
lock = threading.Lock()
# 主循环
# 加载屏幕,字体
print("Loading display Manger")
_dm = dispManger()
print("Loading Font")
_font = ImageFont.truetype('MSYH.TTC', FONT_DEFAULT_SIZE)

# 加载按键
print("loading Keys")
GPIO.setmode(GPIO.BCM)
# 为每个按键设置中断回调函数  
def button_callback(channel):  
    #print(f"Button on channel {channel} pressed")
    if KEY_CONTROL:
        '''
// 退出键
#define RADIO_EXIT_BUTTON  	BUTTON_0_PIN
// 下一首键
#define RADIO_NEXT_BUTTON  	BUTTON_1_PIN
// 上一首键
#define RADIO_PRE_BUTTON	BUTTON_2_PIN
// 暂停/播放键
#define RADIO_ENTER_BUTTON  BUTTON_3_PIN
// 音量+键
#define RADIO_VOLUP_BUTTON  BUTTON_4_PIN
// 音量-键
#define RADIO_VOLDN_BUTTON  BUTTON_5_PIN
// 菜单(选台)键
#define RADIO_MENU_BUTTON   BUTTON_6_PIN
'''

        if channel == Channel_EXIT:
            player.music_exit()
        elif channel == Channel_NEXT:
            player.music_next()
        elif channel == Channel_PRE:
            player.music_front()
        elif channel == Channel_ENTER:
            player.music_pause()
        elif channel == Channel_VOLUP:
            player.music_volup()
        elif channel == Channel_VOLDN:
            player.music_voldn()
# run 'sudo usermod -a -G gpio wiselot' first!
# 初始化GPIO引脚并设置中断  
for pin in PressedPin:  
    GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)  
    GPIO.add_event_detect(pin, GPIO.FALLING, callback=button_callback, bouncetime=200)

print("Loading Done!")
# 加载主进程
print("Starting Player")
player = PlayerMain("/home/wiselot/Music")
player.start()
print("Player Started")

try:  
    # 无限循环等待中断发生  
    while True:  
        time.sleep(0.1)  # 稍微等待一下，但不需要太频繁，因为中断会唤醒程序  
  
except KeyboardInterrupt:  
    # 如果用户按下Ctrl+C，清理GPIO设置  
    GPIO.cleanup()
