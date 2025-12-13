#ifndef RECIPE_CONFIG_H
#define RECIPE_CONFIG_H

#include <Arduino.h>

// ============================================
// 机器状态定义
// ============================================
enum MachineState {
    STATE_IDLE,    // 空置状态
    STATE_RUNNING  // 运行状态
};

// ============================================
// 子阶段定义（每个子状态包含的三个阶段）
// ============================================
enum ProcessPhase {
    PHASE_FILL,    // 进液阶段
    PHASE_AGITATE, // 滚冲阶段
    PHASE_DRAIN    // 出液阶段
};

// ============================================
// 阀门ID定义
// ============================================
enum ValveID {
    VALVE_NONE = 0,   // 无阀门
    VALVE_WATER,      // 水阀
    VALVE_DEV,        // 显影液阀
    VALVE_BLEACH,     // 漂定液阀
    VALVE_FIX,        // 定影液阀
    VALVE_COUNT       // 阀门总数（用于边界检查）
};

// ============================================
// 流程步骤结构体
// ============================================
struct RecipeStep {
    String stepName;      // 屏幕显示的名称，如 "Dev", "Pre-Wash"
    ValveID valve;        // 开启哪个阀门注入液体
    uint32_t processTime; // 滚冲持续时间 (毫秒)
    uint8_t repeatCount;  // 【关键】该步骤重复次数。默认1。水洗设为3或5。
    
    // 构造函数方便初始化
    RecipeStep(String n, ValveID v, uint32_t t, uint8_t r = 1) 
        : stepName(n), valve(v), processTime(t), repeatCount(r) {}
    
    // 默认构造函数
    RecipeStep() 
        : stepName(""), valve(VALVE_NONE), processTime(0), repeatCount(1) {}
};

// ============================================
// 流程配置示例：C-41 流程
// ============================================
// 注意看第 4 步和最后一步的水洗，使用了 repeatCount
const RecipeStep PROCESS_C41[] = {
    // 名称,          阀门,        时间(ms),    重复次数
    {"Pre-Heat",    VALVE_WATER,  60000,      1}, // 温杯预热
    {"Developer",   VALVE_DEV,    210000,     1}, // 显影 3:30
    {"Blix",        VALVE_BLEACH, 390000,     1}, // 漂定 6:30
    {"Washing",     VALVE_WATER,  30000,      5}, // 【关键】水洗：每次30秒，重复5次！
    {"Stabilizer",  VALVE_FIX,    60000,      1}  // 稳定液 (假设接在Fix口)
};

// 获取流程步骤数量
#define PROCESS_C41_STEPS (sizeof(PROCESS_C41) / sizeof(PROCESS_C41[0]))

#endif // RECIPE_CONFIG_H

