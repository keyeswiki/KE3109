from machine import I2C,UART,Pin,PWM
from  Sengo2  import *
import time
import neopixel

# 等待Sengo2完成操作系统的初始化。此等待时间不可去掉，避免出现Sengo2尚未初始化完毕主控器已经开发发送指令的情况
time.sleep(3)

# 选择UART或者I2C通讯模式，Sengo2出厂默认为I2C模式，短按模式按键可以切换
# 4种UART通讯模式：UART9600（标准协议指令），UART57600（标准协议指令），UART115200（标准协议指令），Simple9600（简单协议指令），
#########################################################################################################
# port = UART(2,rx=Pin(16),tx=Pin(17),baudrate=9600)
i2c = I2C(0,scl=Pin(22),sda=Pin(21),freq=400000)

# Sengo2通讯地址：0x60。如果I2C总线挂接多个设备，请避免出现地址冲突
sengo2 = Sengo2(0x60)

err = sengo2.begin(i2c)
print("sengo2.begin: 0x%x"% err)
 
# 1、Sengo2消费版可以储存15张人脸数据
# 2、除了可以通过操作摇杆记忆/删除人脸数据外，还可以通过串口指令进行操作；
# 3、正常使用时，应由主控器发送指令控制Sengo2算法的开启与关闭，而非通过摇杆手动进行操作；
err = sengo2.VisionBegin(sengo2_vision_e.kVisionFace)
print("sengo2.VisionBegin(sengo2_vision_e.kVisionFace):0x%x"% err)

#Initialize the passive buzzer
buzzer = PWM(Pin(2, Pin.OUT))
buzzer.freq(1000)     # 设置默认频率
time.sleep(0.001)
buzzer.duty_u16(0)		# 确保启动时蜂鸣器是关闭的

#Define the number of pin and LEDs connected to neopixel.
pin = Pin(14, Pin.OUT)
np = neopixel.NeoPixel(pin, 4)

# 变量定义
brightness = 100
color_red = 0
color_green = 1
color_blue = 2
color_white = 3
color_off = 4

colors = [
    (brightness, 0, 0),                    # 红
    (0, brightness, 0),                    # 绿
    (0, 0, brightness),                    # 蓝
    (brightness, brightness, brightness),  # 白
    (0, 0, 0)                              # 关闭
]

def set_ws2812_color(color):
    for j in range(4):
        np[j] = colors[color] # 绿
    np.write()
    
set_ws2812_color(color_off)

previousMillis = 0
lastDetectionTime = 0
disappearDelay = 5000  # 1000ms = 1s
currentFaceDetected = False


def play_success_sound():
    """正确提示音（清脆的两短音）"""
    for i in range(2):
        buzzer.freq(1500)    # 1500Hz高频
        buzzer.duty_u16(32768)  # 50%占空比(65536/2)
        time.sleep_ms(100)    # 持续100ms
        buzzer.duty_u16(0)   # 关闭声音
        time.sleep_ms(50)     # 短音间隔50ms

def play_error_sound():
    """错误提示音（低沉的单长音）"""
    buzzer.freq(300)        # 300Hz低频
    buzzer.duty_u16(32768)  # 50%占空比
    time.sleep_ms(500)       # 持续500ms
    buzzer.duty_u16(0)      # 关闭声音


while True:
    # Sengo2不主动返回检测识别结果，需要主控板发送指令进行读取。读取的流程：首先读取识别结果的数量，接收到指令后，Sengo2会刷新结果数据，如果结果数量不为零，那么主控再发送指令读取结果的相关信息。请务必按此流程构建程序。
    obj_num = sengo2.GetValue(sengo2_vision_e.kVisionFace, sentry_obj_info_e.kStatus)
    #获取运行时间
    currentMillis = time.ticks_ms()
    if obj_num:
        for i in range(1,obj_num+1):        
            #人脸标签=0，表示陌生人；人脸标签范围1-15，表示已经记忆的人脸；#人脸标签=200，表示戴口罩；
            l = sengo2.GetValue(sengo2_vision_e.kVisionFace, sentry_obj_info_e.kLabel, i)
            if l == 1 and not currentFaceDetected:
                lastDetectionTime = currentMillis
                currentFaceDetected = True
                # 绿灯闪烁
                set_ws2812_color(color_green)
                play_success_sound()
                time.sleep(0.2)
                # 熄灭
                set_ws2812_color(color_off)
                #播放提示音
            elif l == 0 or l == 200:
                # 红灯闪烁
                set_ws2812_color(color_red)
                play_error_sound()
                time.sleep(0.2)
                # 熄灭
                set_ws2812_color(color_off)
                #播放提示音
            time.sleep(0.2)
          #5秒延时代码
        if currentFaceDetected and (currentMillis - lastDetectionTime >= disappearDelay): 
            currentFaceDetected = False  #退出激活状态
            

