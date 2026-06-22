from machine import I2C,UART,Pin,PWM
from Sengo2 import *
import time

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
 
# 1、sengo2可以识别10张交通卡片
# 2、如需识别自定义的卡片，首先可以尝试集成的深度学习算法；如果效果或者功能不理想，可以购买Sentry2，烧录Sentry2MV固件，自行采集图片训练模型，然后编写python脚本调用模型；
# 4、sengo2最多可以同时识别2排4列共8张卡片，一般按照由上向下、由左向右的顺序输出结果信息；
# 5、正常使用时，应由主控器发送指令控制Sengo2算法的开启与关闭，而非通过摇杆手动进行操作；
# 6、满足特定限制条件下，Sengo2可以并行运行多个识别算法
err = sengo2.VisionBegin(sengo2_vision_e.kVisionCard)
print("sengo2.VisionBegin(sengo2_vision_e.kVisionCard):0x%x"% err)

#Initialize the passive buzzer
buzzer = PWM(Pin(2))

# 电机驱动引脚定义（ESP32 任意输出引脚均可）
pin1 = Pin(32, Pin.OUT)     # 右轮方向
pin2 = PWM(Pin(25), freq=50)  # 右轮速度
pin3 = Pin(33, Pin.OUT)     # 左轮方向
pin4 = PWM(Pin(26), freq=50)  # 左轮速度

#速度变量
right_speed = 50000
left_speed = 50000

# As a function of the car going forward.
def car_forward(): 
  pin1.value(0)
  pin2.duty_u16(right_speed) 
  pin3.value(0)
  pin4.duty_u16(left_speed)

# As a function of the car going backwards.
def car_back(): 
  pin1.value(1)
  pin2.duty_u16(50000 - right_speed)  
  pin3.value(1)
  pin4.duty_u16(50000 - left_speed)

# As a function of the car going left.
def car_left(): 
  pin1.value(0)
  pin2.duty_u16(25000)  
  pin3.value(1)
  pin4.duty_u16(25000)
# As a function of the car going right.
def car_right(): 
  pin1.value(1)
  pin2.duty_u16(25000)  
  pin3.value(0)
  pin4.duty_u16(25000)

# As a function of the car stopping.
def car_stop(): 
  pin1.value(0)
  pin2.duty_u16(0)  
  pin3.value(0)
  pin4.duty_u16(0)

#设置速度为50000的40%，对应速度40的卡片
def speed_40():
    #声明要修改全局变量
    global left_speed,right_speed
    buzzer_play(20000)
    left_speed = int(50000 * 0.4)
    right_speed = int(50000 * 0.4)

#设置速度为50000的60%，对应速度60卡片
def speed_60():
    #声明要修改全局变量
    global left_speed,right_speed
    buzzer_play(30000)
    left_speed = int(50000 * 0.6)
    right_speed = int(50000 * 0.6)

#设置速度为50000的80%，对应速度80卡片
def speed_80():
    #声明要修改全局变量
    global left_speed,right_speed
    buzzer_play(40000)
    left_speed = int(50000 * 0.8)
    right_speed = int(50000 * 0.8)

#设置速度成功提示音
def buzzer_play(speed):
    print("left_speed = %d",left_speed)
    if left_speed != speed:
        buzzer.freq(1000)    # 1000Hz高频
        buzzer.duty_u16(32768)  # 50%占空比(65536/2)
        time.sleep_ms(100)    # 持续100ms
        buzzer.duty_u16(0)   # 关闭声音
        time.sleep_ms(50)     # 短音间隔50ms

while True:
    # Sengo2不主动返回检测识别结果，需要主控板发送指令进行读取。读取的流程：首先读取识别结果的数量，接收到指令后，Sengo2会刷新结果数据，如果结果数量不为零，那么主控再发送指令读取结果的相关信息。请务必按此流程构建程序。
    obj_num = (sengo2.GetValue(sengo2_vision_e.kVisionCard, sentry_obj_info_e.kStatus))
    #判断避免识别到多张卡片时出现混乱
    if obj_num == 1:
        for i in range(1,obj_num+1):
            #获取卡片标签值
            Tags = sengo2.GetValue(sengo2_vision_e.kVisionCard,sentry_obj_info_e.kLabel,i)
            if Tags == 1:
                car_forward()
            elif Tags == 2:
                car_left()
            elif Tags == 3:
                car_right()
            elif Tags == 4:
                car_back()
            elif Tags == 5:
                car_stop()
            elif Tags == 8:
                speed_40()
            elif Tags == 9:
                speed_60()
            elif Tags == 10:
                speed_80()
    else : car_stop()
    time.sleep(0.2)    
