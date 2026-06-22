#include <Arduino.h>          // Arduino核心库
#include <Sentry.h>           // Sentry机器视觉传感器库

// 为Sengo2类型创建别名"Sengo"，简化后续使用
typedef Sengo2 Sengo;

// 定义通信方式（当前启用I2C）
#define SENGO_I2C             
// #define SENGO_UART          // UART串口通信方案（已注释禁用）

// 根据选择的通信方式包含相应库
#ifdef SENGO_I2C
#include <Wire.h>             // I2C通信所需库
#endif

#ifdef SENGO_UART
#include <SoftwareSerial.h>   // 软串口库
#define TX_PIN 11             // 自定义TX引脚号
#define RX_PIN 10             // 自定义RX引脚号
SoftwareSerial mySerial(RX_PIN, TX_PIN);  // 创建软串口对象
#endif

// 定义视觉处理类型为机器学习模式
#define VISION_TYPE Sengo::kVisionLearning  

// 创建Sengo传感器对象实例
Sengo sengo;

void setup() {
  sentry_err_t err = SENTRY_OK;  // 定义错误状态变量，初始化为无错误

  Serial.begin(9600);            // 初始化串口通信，波特率9600
  Serial.println("Waiting for sengo initialize...");  // 打印初始化提示

// 根据选择的通信方式初始化传感器
#ifdef SENGO_I2C
  Wire.begin();                   // 初始化I2C总线
  // 循环尝试连接传感器，直到成功
  while (SENTRY_OK != sengo.begin(&Wire)) { 
    yield();  // 在等待连接期间允许系统处理其他任务
  }
#endif  // SENGO_I2C

#ifdef SENGO_UART
  mySerial.begin(9600);           // 初始化软串口，波特率9600
  // 循环尝试连接传感器，直到成功
  while (SENTRY_OK != sengo.begin(&mySerial)) { 
    yield();  // 在等待连接期间允许系统处理其他任务
  }
#endif  // SENGO_UART

  Serial.println("Sengo begin Success.");  // 打印传感器初始化成功信息
  
  // 启动机器学习视觉识别功能
  err = sengo.VisionBegin(VISION_TYPE);
  
  // 打印视觉识别初始化结果
  Serial.print("sengo.VisionBegin(kVisionLearning) ");
  if (err) {
    Serial.print("Error: 0x");    // 如果出错，打印错误前缀
  } else {
    Serial.print("Success: 0x");  // 如果成功，打印成功前缀
  }
  Serial.println(err, HEX);       // 以16进制格式打印错误代码
}

void loop() {
  // 获取检测到的物体数量（kStatus参数返回检测到的对象总数）
  int obj_num = sengo.GetValue(VISION_TYPE, kStatus);
  
  // 如果检测到至少一个物体
  if (obj_num) {
    // 打印检测到的物体总数
    Serial.print("Totally ");
    Serial.print(obj_num);
    Serial.println(" objects");
    
    // 遍历所有检测到的物体
    for (int i = 1; i <= obj_num; ++i) {
      // 获取当前物体的类别标签（模型训练时定义的类别ID）
      int label = sengo.GetValue(VISION_TYPE, kLabel, i);
      
      // 打印物体序号及其类别标签
      Serial.print("  obj");
      Serial.print(i);
      Serial.print(": ");
      Serial.print("label=");
      Serial.println(label);
    }
  }
  
  delay(200);
}