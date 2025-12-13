#ifndef CONTEXT_H
#define CONTEXT_H

#include "RecipeConfig.h"

// ============================================
// 运行时上下文结构体
// ============================================
struct RuntimeContext {
    // 索引记录
    int currentStepIndex;     // 当前执行配方中的第几步 (0 ~ Total-1)
    int currentRepeatCount;   // 当前步骤的第几次循环 (1 ~ targetRepeat)
    
    // 计时记录
    unsigned long stepStartTime;   // 当前原子动作开始的时间戳
    unsigned long pausedTime;      // 暂停时刻的时间戳（用于断点续传）
    unsigned long totalPausedDuration; // 累计暂停时长（毫秒）
    
    // 状态标志
    bool isPaused;            // 是否处于暂停状态
    MachineState machineState; // 机器当前大状态（空置/运行）
    
    // 微观状态
    ProcessPhase currentPhase; // 目前在注液、滚冲还是排液？
    
    // ============================================
    // 重置上下文到初始状态
    // ============================================
    void reset() {
        currentStepIndex = 0;
        currentRepeatCount = 1;
        stepStartTime = 0;
        pausedTime = 0;
        totalPausedDuration = 0;
        isPaused = false;
        machineState = STATE_IDLE;
        currentPhase = PHASE_FILL; // 从进液阶段开始
    }
    
    // ============================================
    // 获取当前步骤已执行时间（毫秒）
    // 考虑暂停时间
    // ============================================
    unsigned long getElapsedTime() const {
        if (isPaused) {
            return pausedTime - stepStartTime - totalPausedDuration;
        }
        return millis() - stepStartTime - totalPausedDuration;
    }
    
    // ============================================
    // 暂停计时
    // ============================================
    void pause() {
        if (!isPaused) {
            pausedTime = millis();
            isPaused = true;
        }
    }
    
    // ============================================
    // 恢复计时
    // ============================================
    void resume() {
        if (isPaused) {
            unsigned long pauseDuration = millis() - pausedTime;
            totalPausedDuration += pauseDuration;
            isPaused = false;
        }
    }
    
    // ============================================
    // 进入下一个阶段
    // ============================================
    void nextPhase() {
        switch (currentPhase) {
            case PHASE_FILL:
                currentPhase = PHASE_AGITATE;
                break;
            case PHASE_AGITATE:
                currentPhase = PHASE_DRAIN;
                break;
            case PHASE_DRAIN:
                currentPhase = PHASE_FILL; // 准备下一个循环或步骤
                break;
        }
        stepStartTime = millis();
        totalPausedDuration = 0; // 重置暂停累计时间
    }
    
    // ============================================
    // 检查是否完成当前步骤的所有重复
    // ============================================
    bool isStepComplete(const RecipeStep& step) const {
        return currentRepeatCount >= step.repeatCount;
    }
};

// 全局上下文对象
RuntimeContext ctx;

#endif // CONTEXT_H