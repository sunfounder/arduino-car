
#include "Arduino.h"
#include "Arduino_Car.h"
#include "car_control.h"
#include "grayscale.h"
#include "ir_remote.h"
#include "ultrasonic.h"

int motorDirection[] = {1, 0, 0, 1};
int motorPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
int grayscalePins[3] = {A0, A1, A2};
int grayscaleReference = 400;
int ultrasonicPin = 11;
int irObstaclePins[] = {12, A3, A4, A5, 13};
int irReceiverPin = 10;

Car car = Car(motorPins, motorDirection);
Grayscale grayscale = Grayscale(grayscalePins, grayscaleReference);
Ultrasonic ultrasonic = Ultrasonic(ultrasonicPin);
IR_Remote irRemote = IR_Remote(irReceiverPin);

#define CAR_DEFAULT_POWER 40
#define ULTRASONIC_AVOIDANCE_THRESHOLD 20
#define ULTRASONIC_FOLLOW_THRESHOLD 30
#define ULTRASONIC_STOP_THRESHOLD 10
#define NORMAL_LINE_FOLLOW_POWER 30
#define NORMAL_LINE_FOLLOW_ANGLE 90

float current_dir = 0;  //定义小车障碍当前角度初始为0
int angles[6] = {0, 60, 120, 180, 240, 300};  //定义六个角度

int currentMode = MODE_REMOTE_CONTROL;
// State machine for almost all mode. State define see every function
int currentState = 0;

void setup() {
  Serial.begin(9600);  // 115200（PC端使用）
  car.begin();
  grayscale.begin();
  ultrasonic.begin();
  ultrasonic.setRef(ULTRASONIC_AVOIDANCE_THRESHOLD);
  irRemote.begin();
}

void loop() {
  int key = irRemote.read();
  if (key != IR_KEY_ERROR) {
    currentState = 0;
  }
  switch (key) {
    case IR_KEY_POWER:
      break;
    case IR_KEY_MODE:
      break;
    case IR_KEY_MUTE:
      break;
    case IR_KEY_PLAY_PAUSE:
      currentMode = MODE_NORMAL_LINE_FOLLOWING;
      Serial.println("Change mode to normal line following");
      break;
    case IR_KEY_BACKWARD:
      currentMode = MODE_NORMAL_OBSTACLE_FOLLOWING;
      Serial.println("Change mode to normal obstacle following");
      break;
    case IR_KEY_FORWARD:
      currentMode = MODE_NORMAL_OBSTACLE_AVOIDANCE;
      Serial.println("Change mode to normal obstacle avoidance");
      break;
    case IR_KEY_EQ:
      currentMode = MODE_PARALLEL_LINE_FOLLOWING;
      Serial.println("Change mode to parallel line following");
      break;
    case IR_KEY_MINUS:
      currentMode = MODE_PARALLEL_OBSTACLE_FOLLOWING;
      Serial.println("Change mode to parallel obstacle following");
      break;
    case IR_KEY_PLUS:
      currentMode = MODE_PARALLEL_OBSTACLE_AVOIDANCE;
      Serial.println("Change mode to parallel obstacle avoidance");
      break;
    case IR_KEY_0:
    case IR_KEY_CYCLE:
    case IR_KEY_U_SD:
    case IR_KEY_1:
    case IR_KEY_2:
    case IR_KEY_3:
    case IR_KEY_4:
    case IR_KEY_5:
    case IR_KEY_6:
    case IR_KEY_7:
    case IR_KEY_8:
    case IR_KEY_9:
      currentMode = MODE_REMOTE_CONTROL;
      remoteControl(key);
      Serial.println("Change mode to remote control");
      break;
    default:
      break;
  }

  switch (currentMode) {
    case MODE_NORMAL_LINE_FOLLOWING:
      normalLineFollowing();
      break;
    case MODE_NORMAL_OBSTACLE_FOLLOWING:
      normalObstacleFollowing();
      break;
    case MODE_NORMAL_OBSTACLE_AVOIDANCE:
      normalObstacleAvoidance();
      break;
    case MODE_PARALLEL_LINE_FOLLOWING:
      parallelLineFollowing();
      break;
    case MODE_PARALLEL_OBSTACLE_FOLLOWING:
      parallelObstacleFollowing();
      break;
    case MODE_PARALLEL_OBSTACLE_AVOIDANCE:
      parallelObstacleAvoidance();
      break;
    default:
      break;
  }
}

// 循迹传感器呈倒三角形，放在小车正下方。旋转巡线（普通巡线）
void normalLineFollowing() {
  int sensorL, sensorM, sensorR;
  int gsValues[3];
  grayscale.readInto(gsValues);
  sensorL = gsValues[0];
  sensorM = gsValues[1];
  sensorR = gsValues[2];
  Serial.print(sensorL);
  Serial.print(", ");
  Serial.print(sensorM);
  Serial.print(", ");
  Serial.println(sensorR);
  if (sensorL == 0 && sensorM == 1 && sensorR == 0) {
    car.move(0, NORMAL_LINE_FOLLOW_POWER, 0);
  } else if (sensorL == 1 && sensorM == 0 && sensorR == 0) {
    car.move(-NORMAL_LINE_FOLLOW_ANGLE, NORMAL_LINE_FOLLOW_POWER, 0);
  } else if (sensorL == 0 && sensorM == 0 && sensorR == 1) {
    car.move(NORMAL_LINE_FOLLOW_ANGLE, NORMAL_LINE_FOLLOW_POWER, 0);
  } else if (sensorL == 1 && sensorM == 1 && sensorR == 0) {
    car.move(0, 0, NORMAL_LINE_FOLLOW_POWER);
  } else if (sensorL == 0 && sensorM == 1 && sensorR == 1) {
    car.move(0, 0, -NORMAL_LINE_FOLLOW_POWER);
  }
}

// State machine: state 0: follow, state 1: turn to find Obstacle
void normalObstacleFollowing() {
  int sensor_values[5];
  float us_dis;
  us_dis = ultrasonic.read();
  for (int i = 0; i < 5; i++) {
    sensor_values[i] = digitalRead(irObstaclePins[i]);
  }
  if (us_dis < ULTRASONIC_STOP_THRESHOLD) {
    car.stop();
    currentState = 0;
  } else if (us_dis < ULTRASONIC_FOLLOW_THRESHOLD) {
    car.move(0, CAR_DEFAULT_POWER, 0);
    currentState = 0;
  } else if (sensor_values[0] + sensor_values[1] + sensor_values[2] < 3) {
    // if any of the three sensors are triggered, turn left
    car.move(0, 0, -CAR_DEFAULT_POWER);
    currentState = 1;
  } else if (sensor_values[3] + sensor_values[4] < 2) {
    // if any of the two sensors are triggered, turn right
    car.move(0, 0, CAR_DEFAULT_POWER);
    currentState = 1;
  } else {
    if (currentState == 0) {
      car.stop();
    }
  }
}

void normalObstacleAvoidance() {
  int middle_clear = ultrasonic.isClear();
  int left_clear = digitalRead(irObstaclePins[4]);
  int right_clear = digitalRead(irObstaclePins[0]);
  if (middle_clear && left_clear && right_clear) {
    car.move(0, CAR_DEFAULT_POWER, 0);  //前进
  } else {
    if (!left_clear) {
      car.move(0, 0, -CAR_DEFAULT_POWER);
    } else {
      car.move(0, 0, CAR_DEFAULT_POWER);
    }
  }
}

// 循迹传感器呈倒三角形，放在小车正下方。平移巡线
int moveAngle = 0;
int lineFollowOffLineCount = 0;
void parallelLineFollowing() {
  int sensorL, sensorM, sensorR;
  // int count = 0;
  int gsValues[3];
  grayscale.readInto(gsValues, moveAngle);
  sensorL = gsValues[0];
  sensorM = gsValues[1];
  sensorR = gsValues[2];
  if (sensorL == 0 && sensorM == 1 && sensorR == 0) {
    // Serial.println("前进");
    car.move(moveAngle, CAR_DEFAULT_POWER, 0);
  } else if (sensorL == 1 && sensorM == 0 && sensorR == 0) {
    // Serial.println("左移");
    car.move(moveAngle - 60, CAR_DEFAULT_POWER, 0);
  } else if (sensorL == 0 && sensorM == 0 && sensorR == 1) {
    // Serial.println("右移");
    car.move(moveAngle + 60, CAR_DEFAULT_POWER, 0);
  } else if (sensorL == 0 && sensorM == 1 && sensorR == 1) {
    if (moveAngle % 120 == 0) {
      moveAngle += 60;
    } else {
      moveAngle -= 60;
    }
    car.move(moveAngle, CAR_DEFAULT_POWER, 0);
    // Wait until only one sensor is on the line
    while (gsValues[0] + gsValues[1] + gsValues[2] == 1) {
      grayscale.readInto(gsValues, moveAngle);
    }
    Serial.print("改变方向：");
    Serial.println(moveAngle);
  } else if (sensorL == 1 && sensorM == 1 && sensorR == 0) {
    if (moveAngle % 120 == 0) {
      moveAngle -= 60;
    } else {
      moveAngle += 60;
    }
    car.move(moveAngle, CAR_DEFAULT_POWER, 0);
    // Wait until only one sensor is on the line
    while (gsValues[0] + gsValues[1] + gsValues[2] == 1) {
      grayscale.readInto(gsValues, moveAngle);
    }
    Serial.print("改变方向：");
    Serial.println(moveAngle);
  }
  if (sensorL == 0 && sensorM == 0 && sensorR == 0) {
    lineFollowOffLineCount += 1;
    if (lineFollowOffLineCount > 1000) {
      car.stop();
    }
  } else {
    lineFollowOffLineCount = 0;
  }
  moveAngle = moveAngle % 360;
  if (moveAngle < 0) {
    moveAngle += 360;
  }
}

void parallelObstacleFollowing() {
  int sensor_values[5];  //定义五个传感器
  float us_dis;
  int i;
  ultrasonic.read();
  us_dis = ultrasonic.read();  //定义us_dis为超声波的测试距离
  for (i = 0; i < 5; i++) {
    sensor_values[i] =
        digitalRead(irObstaclePins[i]);  //定义传感器起为irObstaclePins[i]的读值
  }
  if (us_dis < ULTRASONIC_AVOIDANCE_THRESHOLD && us_dis > 10) {
    car.move(0, CAR_DEFAULT_POWER, 0);  //前进
  } else if (sensor_values[0] == 0) {
    car.move(60, CAR_DEFAULT_POWER, 0);  //右前进
  } else if (sensor_values[1] == 0) {
    car.move(120, CAR_DEFAULT_POWER, 0);  //右后退
  } else if (sensor_values[2] == 0) {
    car.move(180, CAR_DEFAULT_POWER, 0);  //后退
  } else if (sensor_values[3] == 0) {
    car.move(240, CAR_DEFAULT_POWER, 0);  //左后退
  } else if (sensor_values[4] == 0) {
    car.move(300, CAR_DEFAULT_POWER, 0);  //左前进
  } else if (us_dis < ULTRASONIC_AVOIDANCE_THRESHOLD && sensor_values[0] == 0) {
    car.move(45, CAR_DEFAULT_POWER, 0);  //右前进
  } else if (sensor_values[0] == 0 && sensor_values[1] == 0) {
    car.move(90, CAR_DEFAULT_POWER, 0);  //右转
  } else if (sensor_values[1] == 0 && sensor_values[2] == 0) {
    car.move(150, CAR_DEFAULT_POWER, 0);  //右后退
  } else if (sensor_values[2] == 0 && sensor_values[3] == 0) {
    car.move(210, CAR_DEFAULT_POWER, 0);  //左后退
  } else if (sensor_values[3] == 0 && sensor_values[4] == 0) {
    car.move(270, CAR_DEFAULT_POWER, 0);  //左转
  } else if (sensor_values[4] == 0 && us_dis < ULTRASONIC_AVOIDANCE_THRESHOLD) {
    car.move(330, CAR_DEFAULT_POWER, 0);  //左前进
  } else if (ULTRASONIC_AVOIDANCE_THRESHOLD < us_dis) {
    car.move(0, 0, 0);  //暂停
  }
}

// 平移避障
//      0
//    /
//   5-----1
//      /
//   4-----2
//       /
//      3
int loopOrder[6] = {0, 5, 1, 4, 2, 3};
int currentDir = 0;
void parallelObstacleAvoidance() {
  int sensorValues[6];
  int i, j;
  // Read sensors
  sensorValues[0] = ultrasonic.isClear();  //定义us_dis为超声波的测试距离
  for (i = 0; i < 5; i++) {
    sensorValues[i + 1] = digitalRead(irObstaclePins[i]);
  }
  // Check if current direction, current direction left and current direction
  // right is obstacled
  int currentDirLeft = (currentDir + 5) % 6;
  int currentDirRight = (currentDir + 1) % 6;
  if (sensorValues[currentDir] == 0 || sensorValues[currentDirLeft] == 0 ||
      sensorValues[currentDirRight] == 0) {
    // Check if there is any direction that is not obstacled
    for (i = 0; i < 6; i++) {
      int index = loopOrder[i];
      // Add currentDir to transfer to the current direction
      index = (currentDir + i) % 6;
      // Check if the current direction is clear
      if (sensorValues[index] == 1) {
        // Update currentDir to the current direction
        currentDir = index;
        break;
      }
    }
  }
  // Move
  car.move(currentDir * 60, CAR_DEFAULT_POWER, 0);
}

void remoteControl(long key) {
  switch (key) {
    case IR_KEY_CYCLE:  // Turn Left
      car.move(0, 0, CAR_DEFAULT_POWER);
      Serial.println("Turn Left");
      break;
    case IR_KEY_U_SD:  // Turn Right
      car.move(0, 0, -CAR_DEFAULT_POWER);
      Serial.println("Turn Right");
      break;
    case IR_KEY_1:  // Left Forward
      car.move(315, CAR_DEFAULT_POWER, 0);
      Serial.println("Left Forward");
      break;
    case IR_KEY_2:  // Forward
      car.move(0, CAR_DEFAULT_POWER, 0);
      Serial.println("Forward");
      break;
    case IR_KEY_3:  // Right Forwardå
      car.move(45, CAR_DEFAULT_POWER, 0);
      Serial.println("Right Forward");
      break;
    case IR_KEY_4:  // Left
      car.move(270, CAR_DEFAULT_POWER, 0);
      Serial.println("Left");
      break;
    case IR_KEY_5:  // Stop
      car.stop();
      Serial.println("Stop");
      break;
    case IR_KEY_6:  // Right
      car.move(90, CAR_DEFAULT_POWER, 0);
      Serial.println("Right");
      break;
    case IR_KEY_7:  // Left Backward
      car.move(225, CAR_DEFAULT_POWER, 0);
      Serial.println("Left Backward");
      break;
    case IR_KEY_8:  // Backward
      car.move(180, CAR_DEFAULT_POWER, 0);
      Serial.println("Backward");
      break;
    case IR_KEY_9:  // Right Backward
      car.move(135, CAR_DEFAULT_POWER, 0);
      Serial.println("Right Backward");
      break;
    default:
      car.stop();
      Serial.println("Stop");
      break;
  }
}

void ultrasonicTest() {
  int distance = ultrasonic.read();
  int isObstacle = ultrasonic.isObstacle();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print("cm, Obstacle: ");
  Serial.println(isObstacle);
}

void avoidance() {
  int sensor_values[5];  //定义五个传感器
  bool us_clear;
  int i;
  us_clear = ultrasonic.isClear();  //定义us_clear为超声波的测试距离
  for (i = 0; i < 5; i++) {
    sensor_values[i] =
        digitalRead(irObstaclePins[i]);  //定义传感器起为irObstaclePins[i]的读值
  }
  if (sensor_values[0] == 0 && us_clear && sensor_values[4] == 1 ||
      sensor_values[0] == 0 && !us_clear &&
          sensor_values[4] == 1) {  //向左前旋转
    for (i = 0; i < 200; i++) {
      car.move(0, CAR_DEFAULT_POWER, 50);
      delay(1);
      if (sensor_values[0] == 1) break;
    }
  } else if (sensor_values[0] == 1 && us_clear && sensor_values[4] == 0 ||
             sensor_values[0] == 1 && !us_clear &&
                 sensor_values[4] == 0) {  //向右前旋转
    for (i = 0; i < 200; i++) {
      car.move(0, CAR_DEFAULT_POWER, -50);
      delay(1);
      if (sensor_values[4] == 1) break;
    }
  } else if (sensor_values[0] == 0 && sensor_values[4] == 0) {  //向后旋转
    for (i = 0; i < 2000; i++) {
      car.move(0, 0, -CAR_DEFAULT_POWER);
      delay(1);
      us_clear = ultrasonic.read();  //定义us_clear为超声波的测试距离
      if (us_clear) break;
    }
  } else if (sensor_values[0] == 1 && !us_clear && sensor_values[4] == 1) {
    for (i = 0; i < 200; i++) {
      car.move(0, CAR_DEFAULT_POWER, -50);
      delay(1);
      if (sensor_values[4] == 1) break;
    }
  } else {                              //其他条件
    car.move(0, CAR_DEFAULT_POWER, 0);  //前进
  }
}

void avoidance2() {
  //
  int index_a, index_b;          //定义 a、b 函数
  index_a = floor(current_dir);  // a取当前转向的地板
  index_b = ceil(current_dir);   // b取当前转向的天顶
  int decimal =
      (current_dir - index_a) *
      (angles[index_b] -
       angles
           [index_a]);  //设置decimal函数为（当前转向-去当前转向的地板）*（角度的天顶-角度的地板）
  int current_angle = angles[index_a] + decimal;  //当前角度=角度的地板+decimal
  car.move(current_angle, CAR_DEFAULT_POWER, 0);  //当前前进方向

  //
  int sensor_values[6];  //定义六个传感器
  int temp_values[6];    //定义传感器的临时值
  float us_dis;
  int i;
  int temp_dir_sum = 0;  //定义转向和的临时值
  int temp_count = 0;    //临时计数值
  int dir_sum = 0;       //定义转向的和
  int max_count = 0;     //最大的计算值

  us_dis = ultrasonic.read();  //定义us_dis为超声波的测试距离
  sensor_values[0] = us_dis > ULTRASONIC_AVOIDANCE_THRESHOLD;  //定义第一个传感器为超声波
  for (i = 0; i < 5; i++) {
    sensor_values[i + 1] = digitalRead(
        irObstaclePins[i]);  //定义第二个传感器起为irObstaclePins[i]的读值
  }
  if (sensor_values[index_a] == 0 || sensor_values[index_b] == 0) {  // 有障碍
    // 确定变更的方向
    // 调整sensor 数据顺序
    int temp_index = 0;  // index初始化
    if (sensor_values[index_a] == 0)
      temp_index = index_a;  //重新调整传感器的顺序
    else
      temp_index = index_b;
    for (i = 0; i < 6; i++) {
      int index = (temp_index + i) % 6;
      temp_values[i] = sensor_values[index];
    }
    for (i = 0; i < 6; i++) {
      if (temp_values[i] == 0) {
        if (temp_count > max_count) {
          max_count = temp_count;
          dir_sum = temp_dir_sum;
        }
        temp_dir_sum = 0;
        temp_count = 0;
      } else {
        temp_dir_sum += i + temp_index;
        temp_count++;
      }
    }
    if (temp_count > max_count) {
      max_count = temp_count;
      dir_sum = temp_dir_sum;
    }
    current_dir = dir_sum / max_count;
    if (current_dir >= 6) current_dir -= 6;
  }
}

void lineFollow1() {  //当寻迹传感器碰到黑色线时候会返回高电平信号，
  //其传感器上面灯熄灭。  反之就是低电平。指示灯亮
  int gsValues[3];
  grayscale.readInto(gsValues);
  int i;
  if (gsValues[0] == 0 && gsValues[2] == 1 && gsValues[1] == 0) {
    car.move(0, CAR_DEFAULT_POWER, 0);  //小车前进
  } else if (gsValues[0] == 1 && gsValues[2] == 0 && gsValues[1] == 0) {
    car.move(0, CAR_DEFAULT_POWER, 50);  //小车左旋转
  } else if (gsValues[0] == 0 && gsValues[2] == 0 && gsValues[1] == 1) {
    car.move(0, CAR_DEFAULT_POWER, -50);  //小车右旋转
  } else if (gsValues[0] == 1 && gsValues[2] == 1 && gsValues[1] == 1) {
    car.move(0, 0, 0);  // 小车停止运动
  }
}

void lineFollowTest() {
  int gsValues[3];
  // grayscale.readInto(gsValues);
  grayscale.readRawInto(gsValues);
  Serial.print(gsValues[0]);
  Serial.print(", ");
  Serial.print(gsValues[1]);
  Serial.print(", ");
  Serial.println(gsValues[2]);
}