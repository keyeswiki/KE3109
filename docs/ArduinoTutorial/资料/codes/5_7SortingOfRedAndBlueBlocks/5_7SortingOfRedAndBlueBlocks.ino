#include <Arduino.h>
#include <Sentry.h>  // 引入Sentry机器视觉传感器库

#define servoPin 23 // 舵机引脚

typedef Sengo2 Sengo;  // 为Sengo2类型创建别名Sengo，简化后续使用

// 通信方式选择（当前启用I2C）
#define SENGO_I2C
// #define SENGO_UART   // UART备用选项（已注释）

#ifdef SENGO_I2C
#include <Wire.h>  // I2C通信所需的库
#endif

#ifdef SENGO_UART
#include <SoftwareSerial.h>
#define TX_PIN 11                         // 自定义TX引脚
#define RX_PIN 10                         // 自定义RX引脚
SoftwareSerial mySerial(RX_PIN, TX_PIN);  // 创建软串口对象
#endif

#define VISION_TYPE Sengo::kVisionBlob  // 使用Blob检测（色块识别）
Sengo sengo;                            // 创建Sengo传感器对象

//电机驱动引脚
#define ML 33
#define ML_PWM 26
#define MR 32
#define MR_PWM 25

int set_angle(int angle)
{
  int servo_angle = map(angle, 0, 180, 25, 128);
  return servo_angle;
}


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

  // ============== Blob检测参数配置 ==============
  sengo.SetParamNum(VISION_TYPE, 2);  // 设置检测2种颜色的色块

  sentry_object_t param = { 0 };  // 初始化参数结构体

  /* 设置色块最小检测尺寸（像素）*/
  param.width = 30;   // 最小宽度50像素
  param.height = 30;  // 最小高度50像素

  /* 配置第一个色块（红色）*/
  param.label = Sengo::kColorRed;                // 设置颜色标签为红色
  err = sengo.SetParam(VISION_TYPE, &param, 1);  // 应用到参数1

  if (err) {
    Serial.print("] Error: 0x");
  } else {
    Serial.print("] Success: 0x");
  }
  Serial.println(err, HEX);  // 输出16进制状态码

  /* 配置第二个色块（蓝色）*/
  param.label = Sengo::kColorBlue;               // 设置颜色标签为蓝色
  err = sengo.SetParam(VISION_TYPE, &param, 2);  // 应用到参数2

  if (err) {
    Serial.print("] Error: 0x");
  } else {
    Serial.print("] Success: 0x");
  }
  Serial.println(err, HEX);

  /* 设置每种颜色的最大检测数量 */
  err = sengo.VisionSetMode(VISION_TYPE, 1);  // 最多检测1个色块
  Serial.print("sengo.VisionSetMode");
  if (err) {
    Serial.print(" Error: 0x");
  } else {
    Serial.print(" Success: 0x");
  }
  Serial.println(err, HEX);

  // 启动Blob视觉识别
  err = sengo.VisionBegin(VISION_TYPE);
  Serial.print("sengo.VisionBegin(kVisionBlob) ");
  if (err) {
    Serial.print("Error: 0x");
  } else {
    Serial.print("Success: 0x");
  }
  Serial.println(err, HEX);

  ledcAttachChannel(servoPin,50,10,3);
  ledcWrite(servoPin, set_angle(160));

  pinMode(ML, OUTPUT);      //设置左电机方向控制引脚为输出
  pinMode(ML_PWM, OUTPUT);  //设置左电机方向控制引脚为输出
  pinMode(MR, OUTPUT);      //设置左电机方向控制引脚为输出
  pinMode(MR_PWM, OUTPUT);  //设置左电机方向控制引脚为输出
}

void loop() {
  // 读取检测到的物体总数
  int obj_num = sengo.GetValue(VISION_TYPE, kStatus);

  if (obj_num > 0) {  // 如果检测到物体
    // 遍历所有检测到的色块
    for (int i = 1; i <= obj_num; ++i) {
      int l = sengo.GetValue(VISION_TYPE, kLabel, i);  // 颜色标签
      if (l == 3) {
        sorting(l);
      } else if (l == 5) {
        sorting(1);
      } else car_stop();
    }
  } else car_stop();
}


//色块分拣代码（目的减少重复代码量）
void sorting(int val) {
  //抓住色块
  ledcWrite(servoPin, set_angle(180));
  delay(500);
  //用if区分红色块和蓝色块的分拣方向
  if (val == 3) {
    //左转
    car_left();
  } else {
    //右转
    car_right();
  }
  delay(300);
  //前进
  car_forward();
  delay(300);
  car_stop();
  delay(300);
  //松开色块
  ledcWrite(servoPin, set_angle(160));
  delay(300);
  //后退
  car_back();
  delay(300);
  //用if区分红色块和蓝色块的返回方向
  if (val == 3) {
    //右转
    car_right();
  } else {
    //左转
    car_left();
  }
  delay(300);
}


//小车前进代码
void car_forward() {
  digitalWrite(ML, LOW);
  analogWrite(ML_PWM, 100);
  digitalWrite(MR, LOW);
  analogWrite(MR_PWM, 100);
}

//小车后退代码
void car_back() {
  digitalWrite(ML, HIGH);
  analogWrite(ML_PWM, 150);
  digitalWrite(MR, HIGH);
  analogWrite(MR_PWM, 150);
}

//小车左转代码
void car_left() {
  digitalWrite(ML, HIGH);
  analogWrite(ML_PWM, 155);
  digitalWrite(MR, LOW);
  analogWrite(MR_PWM, 100);
}

//小车右转代码
void car_right() {
  digitalWrite(ML, LOW);
  analogWrite(ML_PWM, 100);
  digitalWrite(MR, HIGH);
  analogWrite(MR_PWM, 155);
}

//小车停止代码
void car_stop() {
  digitalWrite(ML, LOW);
  analogWrite(ML_PWM, 0);
  digitalWrite(MR, LOW);
  analogWrite(MR_PWM, 0);
}
