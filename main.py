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
import shlex
import re
import random

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
# 播放列表浏览
class PlayListView(threading.Thread):
    image = 0
    handle = 0
    draw = 0
    playList = 0
    chose = 0
    MenuSize = 5
    FontSize = 12
    Select = False
    def __init__(self,playList):
        super().__init__()  
        self._stop_event = threading.Event()
        self.playList = playList
        print(len(playList))
        self.image = Image.new('1', (_dm.width, _dm.height))
        self.draw = ImageDraw.Draw(self.image)
        self.handle = _dm.allocateHandle("PlayListView",False)
    def repaint_menu(self):
        self.draw.rectangle((0,_dm.top,_dm.width,_dm.height),outline=0,fill=0)
        for i in range(self.chose,self.chose+min(len(self.playList)-1,self.MenuSize)):
            self.draw.text((3,_dm.top+self.FontSize*(i-self.chose)),'[' + str(i) + ']' + os.path.basename(self.playList[i].content),font=_font,fill=255)
        _dm.refershPanel(self.handle,self.image)
    def do_next_index(self):
        self.chose += 1
        if self.chose >= len(self.playList):
            self.chose = 0
        self.repaint_menu()
    def do_pre_index(self):
        self.chose -= 1
        if self.chose <= 0:
            self.chose = 0
        self.repaint_menu()
    def do_select_exit(self):
        self.Select = True
        #self.stop()
    def do_recover(self):
        self.Select = False
        self.chose = 0
        self.repaint_menu()
    def run(self):
        self.Select = False
        self.repaint_menu()
        while not self._stop_event.is_set():
            time.sleep(0.1)
    def stop(self):  
        self._stop_event.set()

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
    MusicLength = 0
    Volume = 100
    Percent = 0
    # 0: 顺序播放 1: 洗脑循环 2: 随机播放
    PlayMode = 0
    
    def __init__(self,musicDir):
        super().__init__()  
        self._stop_event = threading.Event()
        self.MusicDir = musicDir
        # 扫描文件夹下所有mp3文件
        self.playList = self.loadListFromDirSurface(musicDir)
        #print(self.playList)
        self.image = Image.new('1', (_dm.width, _dm.height))
        self.draw = ImageDraw.Draw(self.image)
        self.handle = _dm.allocateHandle("mainPlayer",True)
        
        # 启动mplayer
        print("Start Mplayer...")
        mplayer_cmd = ['mplayer', '-slave', '-quiet', '-idle']  
        self.MplayerProcess = subprocess.Popen(mplayer_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)  
        if len(self.playList) == 0:
            self.Title = "什么都没有哦"
        else:
            self.music_start_music(self.playList[0])
    
    def send_command(self,mplayer_process, command):  
        if mplayer_process.stdin is not None and not mplayer_process.stdin.closed:  
            mplayer_process.stdin.write(command + '\n')
            mplayer_process.stdin.flush()
    def refersh_TimePos(self):
        self.draw.rectangle((0,_dm.bottom-16,_dm.width,_dm.bottom),outline=0,fill=0)
        self.draw.text((0,_dm.bottom-16),"%2d:%2d/%2d:%2d"%(self.TimePos//60,self.TimePos%60,self.MusicLength//60,self.MusicLength%60),font=_font,fill=255)
        _dm.refershPanel(self.handle,self.image)
    def drawMsgLine(self,lineNum,lineSize,str,sx,sy,fontSize):
        for i in range(0,min(len(str)//lineSize+1,lineNum)):
            self.draw.text((sx,sy+i*fontSize),str[i*lineSize:(i+1)*lineSize],font=_font,fill=255)
    def refersh_Title(self):
        self.draw.rectangle((0,_dm.top,_dm.width,_dm.top+32),outline=0,fill=0)
        #self.draw.text((0,_dm.top),f"{self.Title:<20}",font=_font,fill=255)
        self.drawMsgLine(2,10,self.Title,0,_dm.top,12)
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
    def music_getTitle(self):
        self.send_command(self.MplayerProcess,'get_meta_title')
    def music_getLengthS(self):
        self.send_command(self.MplayerProcess,'get_time_length')
    def music_getTimePosS(self):
        self.send_command(self.MplayerProcess,'get_time_pos')
    def music_getPercent(self):
        self.send_command(self.MplayerProcess,' get_percent_pos')
    def music_exit(self):
        with lock:
            self.stop()
    def music_pause(self):
        with lock:
            self.send_command(self.MplayerProcess,'pause')
            self.Paused = not self.Paused
    def music_start_music(self,ListMusic):
        self.send_command(self.MplayerProcess,'load ' + shlex.quote(ListMusic.content))
        self.music_getLengthS()
        #self.music_getTitle()
        self.Title = os.path.basename(ListMusic.content)
        self.refersh_Title()
    def music_next(self):
        with lock:
            if len(self.playList) == 0 : return
            self.PlayMusic += 1
            if self.PlayMusic >= len(self.playList):
                self.PlayMusic = 0
      #      mixer.music.load(self.playList[self.PlayMusic].content)
            self.music_start_music(self.playList[self.PlayMusic])
                
    def music_front(self):
        if len(self.playList) == 0 : return
        with lock:
            self.PlayMusic -= 1
            if self.PlayMusic <= 0:
                self.PlayMusic = len(self.playList) - 1
       #     mixer.music.load(self.playList[self.PlayMusic].content)
            self.music_start_music(self.playList[self.PlayMusic])
    def music_voldn(self):
        with lock:
            self.Volume -= VOLUME_STEP
            if(self.Volume<0):   self.Volume = 0
        #    mixer.music.set_volume(self.Volume/100)
            self.send_command(self.MplayerProcess,'volume ' + str(self.Volume) + ' 1')
    def music_volup(self):
        with lock:
            self.Volume += VOLUME_STEP
            if(self.Volume>100):   self.Volume = 100
        #    mixer.music.set_volume(self.Volume/100)
            self.send_command(self.MplayerProcess,'volume ' + str(self.Volume) + ' 1')
    def run(self):
        while not self._stop_event.is_set():
            line = self.MplayerProcess.stdout.readline()
            if not line:
                break
            #print(f"mplayer output: {line.strip()}")
            if line.startswith('ANS_LENGTH='):
                match = re.search(r'ANS_LENGTH=(\d+)', line)  
                if match:  
                    self.MusicLength = int(match.group(1))
            elif line.startswith('ANS_TIME_POSITION='):
                match = re.search(r'ANS_TIME_POSITION=(\d+)', line)  
                if match:  
                    self.TimePos = int(match.group(1))
            elif line.startswith('ANS_PERCENT_POSITION='):
                match = re.search(r'ANS_PERCENT_POSITION=(\d+)', line)
                if match:  
                    self.Percent = int(match.group(1))
            if not self.Paused:
                self.music_getTimePosS()
                self.refersh_TimePos()
                self.music_getPercent()
            if self.Percent == 100:
            #if (self.TimePos - self.MusicLength) <= 1 :
                if self.PlayMode == 0:
                    self.music_next()
                elif self.PlayMode == 1:
                    self.music_start_music(self.playList[self.PlayMusic])
                elif self.PlayMode == 2:
                    self.music_start_music(self.playList[random.randint(0,len(self.playList)-1)])
            time.sleep(0.1)
    def stop(self):  
        self._stop_event.set()
        #mixer.music.unload()
        self.send_command(self.MplayerProcess,'quit')

# 为每个按键设置中断回调函数  
def button_callback(channel):  
    global key_control
    global player
    global viewer
    global _dm
    #print(f"Button on channel {channel} pressed")
    if key_control == 1:
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
        elif channel == Channel_MENU:
            key_control = 0
            if viewer == 0:
                viewer = PlayListView(player.playList)
                viewer.start()
            viewer.do_recover()
            _dm.switchCover(viewer.handle)
    else:
        if channel == Channel_EXIT:
            key_control = 1
            _dm.switchCover(player.handle)
        elif channel == Channel_ENTER:
            viewer.do_select_exit()
            key_control = 1
            player.music_start_music(viewer.playList[viewer.chose])
            _dm.switchCover(player.handle)
        elif channel == Channel_NEXT:
            viewer.do_next_index()
        elif channel == Channel_PRE:
            viewer.do_pre_index()    

key_control  = 1
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
# run 'sudo usermod -a -G gpio wiselot' first!
# 初始化GPIO引脚并设置中断  
for pin in PressedPin:  
    GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)  
    GPIO.add_event_detect(pin, GPIO.FALLING, callback=button_callback, bouncetime=300)

print("Loading Done!")
# 加载主进程
print("Starting Player")
player = PlayerMain("/home/wiselot/Music")
player.start()
print("Player Started")
viewer = 0

try:  
    # 无限循环等待中断发生  
    while True:  
        time.sleep(0.1)  # 稍微等待一下，但不需要太频繁，因为中断会唤醒程序  
  
except KeyboardInterrupt:  
    # 如果用户按下Ctrl+C，清理GPIO设置  
    GPIO.cleanup()
