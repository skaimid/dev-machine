#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <AccelStepper.h>
#include "RecipeConfig.h"

// ============================================
// 硬件引脚定义
// ============================================

// BTS7960 出液泵控制引脚
#define DRAIN_PUMP_PWM_PIN 9      // PWM 速度控制
#define DRAIN_PUMP_DIR_PIN 8       // 方向控制（可选）

// A4988 步进电机控制引脚（滚冲）
#define STEPPER_STEP_PIN 2
#define STEPPER_DIR_PIN 3
#define STEPPER_ENABLE_PIN 4

// 继电器控制引脚（5V继电器模块）
#define RELAY_VALVE_WATER 5        // 水阀继电器
#define RELAY_VALVE_DEV 6          // 显影液阀继电器
#define RELAY_VALVE_BLEACH 7       // 漂定液阀继电器
#define RELAY_VALVE_FIX 10         // 定影液阀继电器
#define RELAY_PUMP_FILL 11         // 进液泵继电器

// ============================================
// 硬件配置参数
// ============================================
#define STEPPER_STEPS_PER_REV 200  // 步进电机每圈步数（1.8度）
#define STEPPER_MICROSTEPS 16      // 微步设置（A4988）
#define STEPPER_MAX_SPEED 500.0    // 最大速度（步/秒）
#define STEPPER_ACCELERATION 300.0 // 加速度（步/秒²）
#define AGITATE_CYCLES_PER_MIN 10  // 每分钟滚冲周期数

#define DRAIN_PUMP_SPEED 200       // 出液泵PWM速度（0-255）
#define FILL_TIME 10000           // 进液时间（毫秒）
#define DRAIN_TIME 15000           // 出液时间（毫秒）

// ============================================
// 硬件控制类
// ============================================
class HardwareController {
private:
    AccelStepper stepper;  // 步进电机对象
    
    // 当前状态
    bool isAgitating;
    bool isDraining;
    ValveID currentValve;
    bool pumpRunning;
    
    // 滚冲相关
    unsigned long agitateStartTime;
    unsigned long agitateDuration;
    int agitateDirection;  // 1 或 -1，用于来回滚冲
    
public:
    HardwareController() 
        : stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN),
          isAgitating(false),
          isDraining(false),
          currentValve(VALVE_NONE),
          pumpRunning(false),
          agitateStartTime(0),
          agitateDuration(0),
          agitateDirection(1) {
    }
    
    // ============================================
    // 初始化所有硬件
    // ============================================
    void begin() {
        // 初始化步进电机
        pinMode(STEPPER_ENABLE_PIN, OUTPUT);
        stepper.setMaxSpeed(STEPPER_MAX_SPEED);
        stepper.setAcceleration(STEPPER_ACCELERATION);
        stepper.setEnablePin(STEPPER_ENABLE_PIN);
        stepper.enableOutputs();
        stepper.disableOutputs(); // 初始状态禁用
        
        // 初始化BTS7960出液泵
        pinMode(DRAIN_PUMP_PWM_PIN, OUTPUT);
        pinMode(DRAIN_PUMP_DIR_PIN, OUTPUT);
        digitalWrite(DRAIN_PUMP_DIR_PIN, LOW);
        analogWrite(DRAIN_PUMP_PWM_PIN, 0);
        
        // 初始化所有继电器（低电平触发）
        pinMode(RELAY_VALVE_WATER, OUTPUT);
        pinMode(RELAY_VALVE_DEV, OUTPUT);
        pinMode(RELAY_VALVE_BLEACH, OUTPUT);
        pinMode(RELAY_VALVE_FIX, OUTPUT);
        pinMode(RELAY_PUMP_FILL, OUTPUT);
        
        // 关闭所有继电器
        allValvesOff();
        pumpOff();
        
        Serial.println("Hardware initialized");
    }
    
    // ============================================
    // 关闭所有阀门
    // ============================================
    void allValvesOff() {
        digitalWrite(RELAY_VALVE_WATER, HIGH);   // HIGH = 关闭（假设低电平触发）
        digitalWrite(RELAY_VALVE_DEV, HIGH);
        digitalWrite(RELAY_VALVE_BLEACH, HIGH);
        digitalWrite(RELAY_VALVE_FIX, HIGH);
        currentValve = VALVE_NONE;
    }
    
    // ============================================
    // 打开指定阀门
    // ============================================
    void openValve(ValveID valve) {
        // 先关闭所有阀门
        allValvesOff();
        
        // 打开指定阀门
        switch (valve) {
            case VALVE_WATER:
                digitalWrite(RELAY_VALVE_WATER, LOW);  // LOW = 打开
                break;
            case VALVE_DEV:
                digitalWrite(RELAY_VALVE_DEV, LOW);
                break;
            case VALVE_BLEACH:
                digitalWrite(RELAY_VALVE_BLEACH, LOW);
                break;
            case VALVE_FIX:
                digitalWrite(RELAY_VALVE_FIX, LOW);
                break;
            case VALVE_NONE:
            default:
                break;
        }
        currentValve = valve;
    }
    
    // ============================================
    // 启动进液泵
    // ============================================
    void pumpOn() {
        digitalWrite(RELAY_PUMP_FILL, LOW);  // LOW = 打开
        pumpRunning = true;
    }
    
    // ============================================
    // 关闭进液泵
    // ============================================
    void pumpOff() {
        digitalWrite(RELAY_PUMP_FILL, HIGH);  // HIGH = 关闭
        pumpRunning = false;
    }
    
    // ============================================
    // 开始进液阶段
    // ============================================
    void startFill(ValveID valve) {
        openValve(valve);
        pumpOn();
        Serial.print("Filling: Valve ");
        Serial.println(valve);
    }
    
    // ============================================
    // 停止进液阶段
    // ============================================
    void stopFill() {
        pumpOff();
        allValvesOff();
        Serial.println("Fill stopped");
    }
    
    // ============================================
    // 开始滚冲阶段
    // ============================================
    void startAgitate(unsigned long duration) {
        if (!isAgitating) {
            isAgitating = true;
            agitateStartTime = millis();
            agitateDuration = duration;
            agitateDirection = 1;
            
            // 启用步进电机
            stepper.enableOutputs();
            stepper.setCurrentPosition(0);
            
            Serial.print("Agitating for ");
            Serial.print(duration / 1000);
            Serial.println(" seconds");
        }
    }
    
    // ============================================
    // 更新滚冲（需要在loop中调用）
    // ============================================
    void updateAgitate() {
        if (!isAgitating) return;
        
        // 检查是否超时
        if (millis() - agitateStartTime >= agitateDuration) {
            stopAgitate();
            return;
        }
        
        // 计算滚冲位置（来回运动）
        // 每个周期：前进 -> 后退 -> 前进...
        unsigned long elapsed = millis() - agitateStartTime;
        unsigned long cycleTime = 60000 / AGITATE_CYCLES_PER_MIN; // 每个周期的时间（毫秒）
        unsigned long cyclePosition = elapsed % cycleTime;
        
        // 计算目标位置（来回运动）
        long targetSteps = (STEPPER_STEPS_PER_REV * STEPPER_MICROSTEPS) / 4; // 1/4圈
        long currentTarget;
        
        if (cyclePosition < cycleTime / 2) {
            // 前半周期：前进
            currentTarget = map(cyclePosition, 0, cycleTime / 2, 0, targetSteps);
        } else {
            // 后半周期：后退
            currentTarget = map(cyclePosition, cycleTime / 2, cycleTime, targetSteps, 0);
        }
        
        // 设置目标位置并运行
        stepper.moveTo(currentTarget);
        stepper.run();
    }
    
    // ============================================
    // 停止滚冲阶段
    // ============================================
    void stopAgitate() {
        if (isAgitating) {
            isAgitating = false;
            stepper.disableOutputs();
            stepper.setCurrentPosition(0);
            Serial.println("Agitation stopped");
        }
    }
    
    // ============================================
    // 开始出液阶段
    // ============================================
    void startDrain() {
        if (!isDraining) {
            isDraining = true;
            analogWrite(DRAIN_PUMP_PWM_PIN, DRAIN_PUMP_SPEED);
            digitalWrite(DRAIN_PUMP_DIR_PIN, HIGH);  // 出液方向
            Serial.println("Draining started");
        }
    }
    
    // ============================================
    // 停止出液阶段
    // ============================================
    void stopDrain() {
        if (isDraining) {
            isDraining = false;
            analogWrite(DRAIN_PUMP_PWM_PIN, 0);
            Serial.println("Draining stopped");
        }
    }
    
    // ============================================
    // 停止所有动作（紧急停止）
    // ============================================
    void emergencyStop() {
        stopFill();
        stopAgitate();
        stopDrain();
        Serial.println("Emergency stop activated");
    }
    
    // ============================================
    // 更新硬件状态（需要在loop中调用）
    // ============================================
    void update() {
        if (isAgitating) {
            updateAgitate();
        }
    }
    
    // ============================================
    // 获取当前状态
    // ============================================
    bool getIsAgitating() const { return isAgitating; }
    bool getIsDraining() const { return isDraining; }
    bool getPumpRunning() const { return pumpRunning; }
    ValveID getCurrentValve() const { return currentValve; }
};

// 全局硬件控制器对象
HardwareController hardware;

#endif // HARDWARE_H

