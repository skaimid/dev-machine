#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "RecipeConfig.h"
#include "Recipe.h"
#include "Context.h"

// ============================================
// LCD 显示模块
// ============================================
// 2004 LCD: 20列 x 4行
// I2C地址通常是 0x27 或 0x3F，根据实际情况调整
#define LCD_I2C_ADDRESS 0x27
#define LCD_COLS 20
#define LCD_ROWS 4

// 阶段时间配置（毫秒）
#define PHASE_FILL_TIME 10000    // 进液阶段固定时间：10秒
#define PHASE_DRAIN_TIME 15000   // 排液阶段固定时间：15秒

class DisplayManager {
private:
    LiquidCrystal_I2C lcd;
    unsigned long lastUpdateTime;
    uint8_t selectedRecipeIndex;  // 当前选中的配方索引（用于模式选择）
    const Recipe* currentRecipe;  // 当前运行的配方指针
    
    // ============================================
    // 格式化时间为 MM:SS
    // ============================================
    String formatTime(unsigned long milliseconds) {
        unsigned long totalSeconds = milliseconds / 1000;
        unsigned long minutes = totalSeconds / 60;
        unsigned long seconds = totalSeconds % 60;
        
        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02lu:%02lu", minutes, seconds);
        return String(buffer);
    }
    
    // ============================================
    // 获取阶段名称
    // ============================================
    String getPhaseName(ProcessPhase phase) {
        switch (phase) {
            case PHASE_FILL:
                return "Inject";
            case PHASE_AGITATE:
                return "Agitate";
            case PHASE_DRAIN:
                return "Draining";
            default:
                return "Unknown";
        }
    }
    
    // ============================================
    // 获取机器状态字符串
    // ============================================
    String getStateString() {
        if (ctx.machineState == STATE_IDLE) {
            return "Idle";
        } else if (ctx.isPaused) {
            return "Paused";
        } else if (ctx.currentPhase == PHASE_DRAIN) {
            return "Draining";
        } else {
            return "Running";
        }
    }
    
    // ============================================
    // 显示模式选择界面
    // ============================================
    void showRecipeSelection() {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Select Recipe:");
        
        // 显示当前选中的配方
        if (selectedRecipeIndex < RECIPE_COUNT) {
            lcd.setCursor(0, 1);
            lcd.print("> ");
            lcd.print(AVAILABLE_RECIPES[selectedRecipeIndex].recipeName);
            
            // 显示配方步骤数量
            lcd.setCursor(0, 2);
            lcd.print("Steps: ");
            lcd.print(AVAILABLE_RECIPES[selectedRecipeIndex].stepCount);
            
            // 显示提示信息
            lcd.setCursor(0, 3);
            lcd.print("Use buttons to select");
        } else {
            lcd.setCursor(0, 1);
            lcd.print("No recipe available");
        }
    }
    
    // ============================================
    // 显示运行状态界面
    // ============================================
    void showRunningStatus() {
        if (currentRecipe == nullptr) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Error: No recipe");
            return;
        }
        
        // 第1行：当前模式
        lcd.setCursor(0, 0);
        lcd.print("Mode: ");
        String modeLine = "Mode: " + currentRecipe->recipeName;
        lcd.print(currentRecipe->recipeName);
        // 清除剩余字符
        for (int i = modeLine.length(); i < LCD_COLS; i++) {
            lcd.print(" ");
        }
        
        // 第2行：当前步骤
        lcd.setCursor(0, 1);
        String stepLine = "Step: ";
        if (ctx.currentStepIndex < currentRecipe->stepCount) {
            const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
            String phaseName = getPhaseName(ctx.currentPhase);
            stepLine += step.stepName + " " + phaseName;
            
            // 如果步骤有重复，显示重复次数
            if (step.repeatCount > 1) {
                stepLine += " (" + String(ctx.currentRepeatCount) + "/" + String(step.repeatCount) + ")";
            }
        } else {
            stepLine += "Complete";
        }
        lcd.print(stepLine);
        // 清除剩余字符
        for (int i = stepLine.length(); i < LCD_COLS; i++) {
            lcd.print(" ");
        }
        
        // 第3行：剩余时间
        lcd.setCursor(0, 2);
        String timeLine = "Time: ";
        if (ctx.currentStepIndex < currentRecipe->stepCount) {
            const RecipeStep& step = currentRecipe->getStep(ctx.currentStepIndex);
            unsigned long elapsed = ctx.getElapsedTime();
            unsigned long remaining = 0;
            
            // 根据阶段计算剩余时间
            if (ctx.currentPhase == PHASE_FILL) {
                // 进液阶段：固定时间
                // 注意：这里假设阶段开始时间就是stepStartTime
                // 如果需要更精确，需要在Context中添加phaseStartTime
                unsigned long phaseElapsed = elapsed;
                remaining = (PHASE_FILL_TIME > phaseElapsed) ? (PHASE_FILL_TIME - phaseElapsed) : 0;
            } else if (ctx.currentPhase == PHASE_AGITATE) {
                // 滚冲阶段：使用步骤定义的时间
                // 需要减去进液时间
                unsigned long agitateElapsed = (elapsed > PHASE_FILL_TIME) ? (elapsed - PHASE_FILL_TIME) : 0;
                remaining = (step.processTime > agitateElapsed) ? (step.processTime - agitateElapsed) : 0;
            } else if (ctx.currentPhase == PHASE_DRAIN) {
                // 排液阶段：固定时间
                // 需要减去进液和滚冲时间
                unsigned long drainElapsed = (elapsed > PHASE_FILL_TIME + step.processTime) ? 
                    (elapsed - PHASE_FILL_TIME - step.processTime) : 0;
                remaining = (PHASE_DRAIN_TIME > drainElapsed) ? (PHASE_DRAIN_TIME - drainElapsed) : 0;
            }
            
            timeLine += formatTime(remaining);
        } else {
            timeLine += "00:00";
        }
        lcd.print(timeLine);
        // 清除剩余字符
        for (int i = timeLine.length(); i < LCD_COLS; i++) {
            lcd.print(" ");
        }
        
        // 第4行：机器状态
        lcd.setCursor(0, 3);
        String stateLine = "State: " + getStateString();
        lcd.print(stateLine);
        // 清除剩余字符
        for (int i = stateLine.length(); i < LCD_COLS; i++) {
            lcd.print(" ");
        }
    }

public:
    // ============================================
    // 构造函数
    // ============================================
    DisplayManager() 
        : lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS),
          lastUpdateTime(0),
          selectedRecipeIndex(0),
          currentRecipe(nullptr) {
    }
    
    // ============================================
    // 初始化LCD
    // ============================================
    void begin() {
        lcd.init();
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Dev Machine v1.0");
        delay(1000);
    }
    
    // ============================================
    // 更新显示（每秒刷新）
    // ============================================
    void update() {
        unsigned long now = millis();
        
        // 每秒更新一次
        if (now - lastUpdateTime >= 1000) {
            lastUpdateTime = now;
            
            if (ctx.machineState == STATE_IDLE) {
                showRecipeSelection();
            } else {
                showRunningStatus();
            }
        }
    }
    
    // ============================================
    // 设置当前运行的配方
    // ============================================
    void setCurrentRecipe(const Recipe* recipe) {
        currentRecipe = recipe;
        // 立即更新显示
        lastUpdateTime = 0;
        update();
    }
    
    // ============================================
    // 获取当前选中的配方索引（用于模式选择）
    // ============================================
    uint8_t getSelectedRecipeIndex() const {
        return selectedRecipeIndex;
    }
    
    // ============================================
    // 设置选中的配方索引（用于模式选择）
    // ============================================
    void setSelectedRecipeIndex(uint8_t index) {
        if (index < RECIPE_COUNT) {
            selectedRecipeIndex = index;
            // 立即更新显示
            lastUpdateTime = 0;
            update();
        }
    }
    
    // ============================================
    // 选择下一个配方（循环）
    // ============================================
    void selectNextRecipe() {
        selectedRecipeIndex = (selectedRecipeIndex + 1) % RECIPE_COUNT;
        lastUpdateTime = 0;
        update();
    }
    
    // ============================================
    // 选择上一个配方（循环）
    // ============================================
    void selectPreviousRecipe() {
        selectedRecipeIndex = (selectedRecipeIndex + RECIPE_COUNT - 1) % RECIPE_COUNT;
        lastUpdateTime = 0;
        update();
    }
    
    // ============================================
    // 强制立即刷新显示
    // ============================================
    void forceUpdate() {
        lastUpdateTime = 0;
        update();
    }
    
    // ============================================
    // 清除显示
    // ============================================
    void clear() {
        lcd.clear();
    }
    
    // ============================================
    // 显示自定义消息（用于调试或提示）
    // ============================================
    void showMessage(const String& line1, const String& line2 = "", 
                     const String& line3 = "", const String& line4 = "") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(line1);
        if (line2.length() > 0) {
            lcd.setCursor(0, 1);
            lcd.print(line2);
        }
        if (line3.length() > 0) {
            lcd.setCursor(0, 2);
            lcd.print(line3);
        }
        if (line4.length() > 0) {
            lcd.setCursor(0, 3);
            lcd.print(line4);
        }
    }
};

// 全局显示管理器对象
DisplayManager display;

#endif // DISPLAY_H

