// ============================================
// 自动滚冲机主程序
// ============================================
// 
// 硬件连接说明：
// - BTS7960 出液泵：PWM引脚9，方向引脚8
// - A4988 步进电机（滚冲）：步进引脚2，方向引脚3，使能引脚4
// - 5V继电器模块：
//   * 水阀：引脚5
//   * 显影液阀：引脚6
//   * 漂定液阀：引脚7
//   * 定影液阀：引脚10
//   * 进液泵：引脚11
// - 2004 I2C LCD：SDA/SCL（根据Arduino型号）
//
// 使用说明：
// 1. 在IDLE状态下，可以通过按钮选择配方（需要自行实现按钮检测）
// 2. 调用 startRecipe() 开始运行选中的配方
// 3. 调用 togglePause() 暂停/恢复流程
// 4. 调用 emergencyStop() 紧急停止
//
// ============================================

#include "RecipeConfig.h"
#include "Recipe.h"
#include "Context.h"
#include "Display.h"
#include "Hardware.h"

// ============================================
// 全局变量
// ============================================
const Recipe* currentRecipe = nullptr;  // 当前运行的配方

// 按钮引脚定义（可根据实际情况修改）
#define BUTTON_START 12      // 开始/暂停按钮
#define BUTTON_SELECT 13     // 选择配方按钮
#define BUTTON_EMERGENCY A0  // 紧急停止按钮（模拟引脚）

// ============================================
// 辅助函数声明
// ============================================
void startRecipe();
void processRunningState();
void handlePhaseTransition();
bool checkPhaseComplete();
void completeCurrentStep();
void completeRecipe();

// ============================================
// 主程序初始化
// ============================================
void setup() {
    // 初始化串口（用于调试）
    Serial.begin(9600);
    delay(500);
    
    // 初始化硬件
    hardware.begin();
    
    // 初始化显示模块
    display.begin();
    
    // 初始化上下文
    ctx.reset();
    
    // 设置默认选中的配方
    display.setSelectedRecipeIndex(0);
    
    // 初始化按钮（如果需要）
    pinMode(BUTTON_START, INPUT_PULLUP);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);
    pinMode(BUTTON_EMERGENCY, INPUT_PULLUP);
    
    Serial.println("=================================");
    Serial.println("Dev Machine initialized");
    Serial.println("Ready to start");
    Serial.println("=================================");
}

// ============================================
// 主循环
// ============================================
void loop() {
    // 更新硬件状态（步进电机等）
    hardware.update();
    
    // 更新显示（每秒自动刷新）
    display.update();
    
    // 检查紧急停止按钮（最高优先级）
    if (digitalRead(BUTTON_EMERGENCY) == LOW) {
        emergencyStop();
        delay(500); // 防抖
        return;
    }
    
    // 根据机器状态执行相应逻辑
    if (ctx.machineState == STATE_IDLE) {
        // IDLE状态：等待用户操作
        // 检查选择按钮（切换配方）
        static unsigned long lastSelectPress = 0;
        if (digitalRead(BUTTON_SELECT) == LOW) {
            if (millis() - lastSelectPress > 300) { // 防抖
                display.selectNextRecipe();
                lastSelectPress = millis();
            }
        }
        
        // 检查开始按钮
        static unsigned long lastStartPress = 0;
        if (digitalRead(BUTTON_START) == LOW) {
            if (millis() - lastStartPress > 300) { // 防抖
                startRecipe();
                lastStartPress = millis();
            }
        }
        
    } else if (ctx.machineState == STATE_RUNNING) {
        // RUNNING状态：执行流程控制
        processRunningState();
        
        // 检查暂停/恢复按钮
        static unsigned long lastPausePress = 0;
        if (digitalRead(BUTTON_START) == LOW) {
            if (millis() - lastPausePress > 300) { // 防抖
                togglePause();
                lastPausePress = millis();
            }
        }
    }
    
    // 短暂延迟，避免CPU占用过高
    delay(10);
}

// ============================================
// 开始运行配方
// ============================================
void startRecipe() {
    // 获取选中的配方
    uint8_t selectedIndex = display.getSelectedRecipeIndex();
    currentRecipe = getRecipeByIndex(selectedIndex);
    
    if (currentRecipe == nullptr) {
        Serial.println("Error: Invalid recipe selected");
        return;
    }
    
    // 重置上下文
    ctx.reset();
    ctx.machineState = STATE_RUNNING;
    ctx.currentStepIndex = 0;
    ctx.currentRepeatCount = 1;
    ctx.currentPhase = PHASE_FILL;
    ctx.stepStartTime = millis();
    
    // 设置显示
    display.setCurrentRecipe(currentRecipe);
    
    Serial.println("=================================");
    Serial.print("Starting recipe: ");
    Serial.println(currentRecipe->recipeName);
    Serial.println("=================================");
    
    // 开始第一个阶段的进液
    if (ctx.currentStepIndex < currentRecipe->stepCount) {
        const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
        hardware.startFill(step.valve);
    }
}

// ============================================
// 处理运行状态
// ============================================
void processRunningState() {
    if (currentRecipe == nullptr) {
        Serial.println("Error: No recipe loaded");
        ctx.machineState = STATE_IDLE;
        return;
    }
    
    // 检查是否完成所有步骤
    if (ctx.currentStepIndex >= currentRecipe->stepCount) {
        completeRecipe();
        return;
    }
    
    // 如果暂停，不执行任何操作
    if (ctx.isPaused) {
        return;
    }
    
    // 检查当前阶段是否完成
    if (checkPhaseComplete()) {
        handlePhaseTransition();
    }
}

// ============================================
// 检查阶段是否完成
// ============================================
bool checkPhaseComplete() {
    if (ctx.currentStepIndex >= currentRecipe->stepCount) {
        return false;
    }
    
    const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
    unsigned long elapsed = ctx.getElapsedTime();
    
    // stepStartTime 在每个阶段开始时重置，所以 elapsed 是当前阶段的已用时间
    switch (ctx.currentPhase) {
        case PHASE_FILL:
            // 进液阶段：固定时间
            return elapsed >= FILL_TIME;
            
        case PHASE_AGITATE:
            // 滚冲阶段：使用步骤定义的时间
            return elapsed >= step.processTime;
            
        case PHASE_DRAIN:
            // 出液阶段：固定时间
            return elapsed >= DRAIN_TIME;
            
        default:
            return false;
    }
}

// ============================================
// 处理阶段转换
// ============================================
void handlePhaseTransition() {
    if (ctx.currentStepIndex >= currentRecipe->stepCount) {
        return;
    }
    
    const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
    
    // 停止当前阶段
    switch (ctx.currentPhase) {
        case PHASE_FILL:
            hardware.stopFill();
            Serial.println("Phase: FILL -> AGITATE");
            break;
            
        case PHASE_AGITATE:
            hardware.stopAgitate();
            Serial.println("Phase: AGITATE -> DRAIN");
            break;
            
        case PHASE_DRAIN:
            hardware.stopDrain();
            Serial.println("Phase: DRAIN -> Complete Step");
            // 出液完成后，完成当前步骤
            completeCurrentStep();
            return; // 直接返回，completeCurrentStep 会处理下一步
    }
    
    // 进入下一个阶段
    ctx.nextPhase();
    
    // 根据新阶段启动相应硬件
    switch (ctx.currentPhase) {
        case PHASE_FILL:
            // 如果是新的步骤，需要重新进液
            // 如果是重复循环，也需要重新进液
            hardware.startFill(step.valve);
            Serial.print("Starting fill for step: ");
            Serial.println(step.stepName);
            break;
            
        case PHASE_AGITATE:
            // 开始滚冲
            hardware.startAgitate(step.processTime);
            Serial.print("Starting agitation for: ");
            Serial.print(step.processTime / 1000);
            Serial.println(" seconds");
            break;
            
        case PHASE_DRAIN:
            // 开始出液
            hardware.startDrain();
            Serial.println("Starting drain");
            break;
    }
}

// ============================================
// 完成当前步骤
// ============================================
void completeCurrentStep() {
    if (ctx.currentStepIndex >= currentRecipe->stepCount) {
        return;
    }
    
    const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
    
    // 检查是否完成所有重复
    if (ctx.isStepComplete(step)) {
        // 步骤完成，进入下一步
        ctx.currentStepIndex++;
        ctx.currentRepeatCount = 1;
        ctx.currentPhase = PHASE_FILL;
        ctx.stepStartTime = millis();
        ctx.totalPausedDuration = 0;
        
        Serial.print("Step completed: ");
        Serial.println(step.stepName);
        
        // 如果还有下一步，开始进液
        if (ctx.currentStepIndex < currentRecipe->stepCount) {
            const RecipeStep& nextStep = currentRecipe->getStep(ctx.currentStepIndex);
            hardware.startFill(nextStep.valve);
            Serial.print("Starting next step: ");
            Serial.println(nextStep.stepName);
        }
    } else {
        // 需要重复当前步骤
        ctx.currentRepeatCount++;
        ctx.currentPhase = PHASE_FILL;
        ctx.stepStartTime = millis();
        ctx.totalPausedDuration = 0;
        
        Serial.print("Repeating step: ");
        Serial.print(step.stepName);
        Serial.print(" (");
        Serial.print(ctx.currentRepeatCount);
        Serial.print("/");
        Serial.print(step.repeatCount);
        Serial.println(")");
        
        // 重新开始进液
        hardware.startFill(step.valve);
    }
}

// ============================================
// 完成整个配方
// ============================================
void completeRecipe() {
    // 停止所有硬件
    hardware.emergencyStop();
    
    // 重置状态
    ctx.machineState = STATE_IDLE;
    ctx.reset();
    currentRecipe = nullptr;
    
    // 更新显示
    display.setCurrentRecipe(nullptr);
    display.forceUpdate();
    
    Serial.println("=================================");
    Serial.println("Recipe completed!");
    Serial.println("=================================");
    
    // 显示完成消息
    display.showMessage("Recipe Complete!", "All steps finished", "", "Press to restart");
    delay(3000);
}

// ============================================
// 暂停/恢复功能（可在按钮中断中调用）
// ============================================
void togglePause() {
    if (ctx.machineState == STATE_RUNNING) {
        if (ctx.isPaused) {
            ctx.resume();
            Serial.println("Resumed");
            
            // 根据当前阶段恢复硬件
            if (ctx.currentStepIndex < currentRecipe->stepCount) {
                const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
                switch (ctx.currentPhase) {
                    case PHASE_FILL:
                        hardware.startFill(step.valve);
                        break;
                    case PHASE_AGITATE:
                        hardware.startAgitate(step.processTime);
                        break;
                    case PHASE_DRAIN:
                        hardware.startDrain();
                        break;
                }
            }
        } else {
            ctx.pause();
            hardware.emergencyStop();
            Serial.println("Paused");
        }
    }
}

// ============================================
// 紧急停止功能
// ============================================
void emergencyStop() {
    hardware.emergencyStop();
    ctx.machineState = STATE_IDLE;
    ctx.reset();
    currentRecipe = nullptr;
    display.setCurrentRecipe(nullptr);
    Serial.println("Emergency stop!");
}