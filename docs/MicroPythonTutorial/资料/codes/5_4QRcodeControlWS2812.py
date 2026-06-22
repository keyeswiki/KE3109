from machine import I2C,UART,Pin
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

 
# 1、Sengo2只能识别并解码一个由不超过10个字符生成的二维码；
# 2、Apriltag与二维码不同，请勿混淆。二者在使用时，周围一圈均需要留白；
# 3、正常使用时，应由主控器发送指令控制Sentry2算法的开启与关闭，而非通过摇杆手动进行操作；
# 4、满足特定限制条件下，Sentry2可以并行运行多个识别算法；
err = sengo2.VisionBegin(sengo2_vision_e.kVisionQrCode)
print("sengo2.VisionBegin(sengo2_vision_e.kVisionQrCode):0x%x"% err)

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

lastDetectionTime = 0

while True:
    # Sengo2不主动返回检测识别结果，需要主控板发送指令进行读取。读取的流程：首先读取识别结果的数量，接收到指令后，Sengo2会刷新结果数据，如果结果数量不为零，那么主控再发送指令读取结果的相关信息。请务必按此流程构建程序。
    obj_num = sengo2.GetValue(sengo2_vision_e.kVisionQrCode, sentry_obj_info_e.kStatus)
    # Sengo2只可识别并解码一个由不超过10个字符生成的二维码；所以返回结果非0时，只要获取并处理第1个结果的相关数据即可
    #获取运行时间
    currentMillis = time.ticks_ms()
    if obj_num:
        lastDetectionTime = currentMillis
        QRcodeStr = sengo2.GetQrCodeValue()
        if QRcodeStr == "Red":
            set_ws2812_color(color_red)
        elif QRcodeStr == "Green":
            set_ws2812_color(color_green)
        elif QRcodeStr == "Blue":
            set_ws2812_color(color_blue)
        elif QRcodeStr == "Black":
            set_ws2812_color(color_off)
        elif QRcodeStr == "White":
            set_ws2812_color(color_white)
    # 如果5秒内没有检测到二维码将会熄灭WS2812灯
    if currentMillis - lastDetectionTime >= 5000:
        lastDetectionTime = currentMillis
        set_ws2812_color(color_off)

