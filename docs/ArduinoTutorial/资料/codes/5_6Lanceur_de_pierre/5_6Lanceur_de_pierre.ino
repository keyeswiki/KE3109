// 引入 Arduino 核心库
#include <Arduino.h>
// 引入 Sentry 视觉传感器库
#include <Sentry.h>


#define servoPin 23 // 舵机引脚

#define BUZZER_PIN 2 //蜂鸣器引脚

// 定义 Sengo2 类型的别名（简化使用）
typedef Sengo2 Sengo;

// 选择通信方式（当前启用 I2C）
#define SENGO_I2C
// #define SENGO_UART  // UART 方式被注释掉

// 根据选择的通信方式包含相应库
#ifdef SENGO_I2C
#include <Wire.h>  // I2C 通信库
#endif
#ifdef SENGO_UART
#include <SoftwareSerial.h>
#define TX_PIN 11  // 软件串口发送引脚
#define RX_PIN 10  // 软件串口接收引脚
SoftwareSerial mySerial(RX_PIN, TX_PIN);  // 创建软件串口对象
#endif

// 设置视觉识别类型为 20 类物体识别
#define VISION_TYPE Sengo::kVision20Classes
Sengo sengo;  // 创建 Sentry 传感器对象

// 全局状态变量
unsigned long lastDetectionTime = 0;        // 最后检测到瓶子的时间戳
const unsigned long disappearDelay = 2000;  // 投掷间歇时间
bool currentBottleDetected = false;  // 是否检测到瓶子

int set_angle(int angle)
{
  int servo_angle = map(angle, 0, 180, 25, 128);
  return servo_angle;
}

// 初始化设置（Arduino 启动时执行一次）
void setup() {
    sentry_err_t err = SENTRY_OK;  // 错误状态变量

    Serial.begin(9600);  // 初始化串口通信（用于调试输出）
    Serial.println("Waiting for sengo initialize...");

    // I2C 初始化流程
#ifdef SENGO_I2C
    Wire.begin();  // 初始化 I2C 总线
    // 循环等待传感器初始化成功
    while (SENTRY_OK != sengo.begin(&Wire)) { 
        yield();  // 在等待期间让出 CPU 控制权
    }
#endif

    // UART 初始化流程（当前未启用）
#ifdef SENGO_UART
    mySerial.begin(9600);
    while (SENTRY_OK != sengo.begin(&mySerial)) { 
        yield();
    }
#endif

    Serial.println("Sengo begin Success.");

    // 启动视觉识别功能（20 类物体检测）
    err = sengo.VisionBegin(VISION_TYPE);
    
    // 打印初始化结果
    Serial.print("sengo.VisionBegin(kVision20Classes) ");
    if (err) {
        Serial.print("Error: 0x");
    } else {
        Serial.print("Success: 0x");
    }
    Serial.println(err, HEX);  // 十六进制输出错误码

  ledcAttachChannel(servoPin,50,10,3);
  ledcWrite(servoPin, set_angle(35));

    pinMode(BUZZER_PIN,OUTPUT);
}

// 主循环（重复执行）
void loop() {
    // 获取检测到的物体数量（kStatus 表示查询状态）
    int obj_num = sengo.GetValue(VISION_TYPE, kStatus);
    unsigned long currentMillis = millis();

    
    if (obj_num) {  // 如果检测到物体
        Serial.print("Totally ");
        Serial.print(obj_num);
        Serial.println(" objects");
        
        // 遍历所有检测到的物体
        for (int i = 1; i <= obj_num; ++i) {
            // 获取物体类别标签
            int l = sengo.GetValue(VISION_TYPE, kLabel, i);  // 物体类别标签索引
            if(l == 5 && !currentBottleDetected){
            lastDetectionTime = currentMillis;  // 更新最后检测时间
              //投掷倒计时声音
              countdown(3);
              //投掷
              ledcWrite(servoPin, set_angle(90));
              delay(1000);
              //缓慢回落
              for(int j = 90;j > 35;j--){
                ledcWrite(servoPin, set_angle(j));
                delay(15);
              }
              delay(500);
            }
        }
    }
    if((currentMillis - lastDetectionTime) > disappearDelay){
      currentBottleDetected = false; 
    }
}

void countdown(int seconds) {
  for (int i = seconds; i > 0; i--) {
    // 倒计时滴答声
    tone(BUZZER_PIN, 800, 100);
    delay(200);
    noTone(BUZZER_PIN);

    // 间隔时间
    delay(800);
  }
}