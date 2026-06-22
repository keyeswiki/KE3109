#include <Arduino.h>
#include <Sentry.h>  // 引入Sentry机器视觉库

typedef Sengo2 Sengo;  // 为Sengo2类型创建别名Sengo，简化使用

// 通信方式选择（当前启用I2C）
#define SENGO_I2C
// #define SENGO_UART  // UART备用选项（已注释）

#ifdef SENGO_I2C
#include <Wire.h>  // I2C通信所需库
#endif

#ifdef SENGO_UART
#include <SoftwareSerial.h>
#define TX_PIN 11  // 自定义TX引脚
#define RX_PIN 10  // 自定义RX引脚
SoftwareSerial mySerial(RX_PIN, TX_PIN);  // 创建软串口对象
#endif

#define VISION_TYPE Sengo::kVisionAprilTag  // 使用AprilTag检测模式
Sengo sengo;  // 创建Sengo传感器对象

void setup() {
  sentry_err_t err = SENTRY_OK;  // 错误状态变量

  Serial.begin(9600);  // 初始化串口用于调试输出
  Serial.println("Waiting for sengo initialize...");

// 根据选择的通信方式初始化传感器
#ifdef SENGO_I2C
  Wire.begin();  // 初始化I2C总线
  // 持续尝试连接直到成功
  while (SENTRY_OK != sengo.begin(&Wire)) {
    yield();  // 在等待时允许其他任务运行
  }
#endif

#ifdef SENGO_UART
  mySerial.begin(9600);
  while (SENTRY_OK != sengo.begin(&mySerial)) {
    yield();
  }
#endif

  Serial.println("Sengo begin Success.");
  
  // ============== AprilTag检测配置 ==============
  
  // 设置AprilTag家族类型为16H5（标准标签集）
  err = sengo.VisionSetMode(VISION_TYPE, Sengo::kVisionModeFamily16H5);
  
  // 设置检测级别为速度优先（牺牲精度提高速度）
  err = sengo.VisionSetLevel(VISION_TYPE, kLevelSpeed);
  
  if (err) {
    Serial.print("sengo.VisionSetMode Error: 0x");
    Serial.println(err, HEX);  // 输出错误代码
  }
  
  // 启动AprilTag视觉识别
  err = sengo.VisionBegin(VISION_TYPE);
  Serial.print("sengo.VisionBegin(kVisionAprilTag) ");
  if (err) {
    Serial.print("Error: 0x");
  } else {
    Serial.print("Success: 0x");
  }
  Serial.println(err, HEX);  // 输出初始化结果
}

void loop() {
  // 读取检测到的AprilTag数量
  int obj_num = sengo.GetValue(VISION_TYPE, kStatus);
  
  if (obj_num > 0) {  // 如果检测到AprilTag
    Serial.print("Totally ");
    Serial.print(obj_num);
    Serial.println(" tags detected");
    
    // 遍历所有检测到的标签
    for (int i = 1; i <= obj_num; ++i) {
      // 获取标签中心位置
      int x = sengo.GetValue(VISION_TYPE, kXValue, i);
      int y = sengo.GetValue(VISION_TYPE, kYValue, i);
      
      // 获取标签边界框尺寸
      int w = sengo.GetValue(VISION_TYPE, kWidthValue, i);
      int h = sengo.GetValue(VISION_TYPE, kHeightValue, i);
      
      // 获取标签ID（唯一标识符）
      int tag_id = sengo.GetValue(VISION_TYPE, kLabel, i);
      
      // 格式化输出检测结果
      Serial.print("  Tag #");
      Serial.print(i);
      Serial.print(": ");
      Serial.print("ID=");
      Serial.print(tag_id);
      Serial.print(", Position=(");
      Serial.print(x);
      Serial.print(",");
      Serial.print(y);
      Serial.print("), Size=");
      Serial.print(w);
      Serial.print("x");
      Serial.print(h);
      Serial.println();
    }
  }
  
  delay(100);  // 适当延时，降低刷新率
}