#ifndef RECIPE_H
#define RECIPE_H

#include "RecipeConfig.h"

// ============================================
// 配方结构体
// ============================================
struct Recipe {
    String recipeName;           // 配方名称，用于显示，如 "C-41", "E-6", "B&W"
    const RecipeStep* steps;     // 步骤数组指针
    uint8_t stepCount;           // 步骤数量
    
    // 构造函数
    Recipe(String name, const RecipeStep* s, uint8_t count)
        : recipeName(name), steps(s), stepCount(count) {}
    
    // 默认构造函数
    Recipe()
        : recipeName(""), steps(nullptr), stepCount(0) {}
    
    // 获取指定步骤
    const RecipeStep& getStep(uint8_t index) const {
        return steps[index];
    }
    
    // 检查索引是否有效
    bool isValidStep(uint8_t index) const {
        return index < stepCount;
    }
};

// ============================================
// C-41 彩色负片流程
// ============================================
const RecipeStep STEPS_C41[] = {
    // 名称,          阀门,        时间(ms),    重复次数
    {"Pre-Heat",    VALVE_WATER,  60000,      1}, // 温杯预热
    {"Developer",   VALVE_DEV,    210000,     1}, // 显影 3:30
    {"Blix",        VALVE_BLEACH, 390000,     1}, // 漂定 6:30
    {"Washing",     VALVE_WATER,  30000,      5}, // 【关键】水洗：每次30秒，重复5次！
    {"Stabilizer",  VALVE_FIX,    60000,      1}  // 稳定液
};

// ============================================
// E-6 彩色反转片流程（示例）
// ============================================
const RecipeStep STEPS_E6[] = {
    {"Pre-Heat",    VALVE_WATER,  60000,      1}, // 温杯预热
    {"First Dev",   VALVE_DEV,    360000,     1}, // 首显 6:00
    {"Washing",     VALVE_WATER,  30000,      3}, // 水洗
    {"Reversal",    VALVE_BLEACH, 120000,     1}, // 反转
    {"Color Dev",   VALVE_DEV,    360000,     1}, // 彩显 6:00
    {"Washing",     VALVE_WATER,  30000,      3}, // 水洗
    {"Bleach",      VALVE_BLEACH, 360000,     1}, // 漂白 6:00
    {"Washing",     VALVE_WATER,  30000,      3}, // 水洗
    {"Fixer",       VALVE_FIX,    240000,     1}, // 定影 4:00
    {"Washing",     VALVE_WATER,  30000,      5}, // 水洗
    {"Stabilizer",  VALVE_FIX,    60000,      1}  // 稳定液
};

// ============================================
// 黑白负片流程（示例）
// ============================================
const RecipeStep STEPS_BW[] = {
    {"Pre-Heat",    VALVE_WATER,  60000,      1}, // 温杯预热
    {"Developer",   VALVE_DEV,    480000,     1}, // 显影 8:00
    {"Stop Bath",   VALVE_WATER,  30000,      1}, // 停显
    {"Washing",     VALVE_WATER,  30000,      3}, // 水洗
    {"Fixer",       VALVE_FIX,    300000,     1}, // 定影 5:00
    {"Washing",     VALVE_WATER,  30000,      5}, // 水洗
    {"Stabilizer",  VALVE_FIX,    60000,      1}  // 稳定液
};

// ============================================
// 所有可用配方列表
// ============================================
const Recipe AVAILABLE_RECIPES[] = {
    {"C-41",        STEPS_C41, sizeof(STEPS_C41) / sizeof(STEPS_C41[0])},
    {"E-6",         STEPS_E6,  sizeof(STEPS_E6) / sizeof(STEPS_E6[0])},
    {"B&W",         STEPS_BW,  sizeof(STEPS_BW) / sizeof(STEPS_BW[0])}
};

// 配方总数
#define RECIPE_COUNT (sizeof(AVAILABLE_RECIPES) / sizeof(AVAILABLE_RECIPES[0]))

// ============================================
// 配方管理辅助函数
// ============================================

// 根据名称查找配方
const Recipe* findRecipeByName(const String& name) {
    for (uint8_t i = 0; i < RECIPE_COUNT; i++) {
        if (AVAILABLE_RECIPES[i].recipeName == name) {
            return &AVAILABLE_RECIPES[i];
        }
    }
    return nullptr; // 未找到
}

// 根据索引获取配方
const Recipe* getRecipeByIndex(uint8_t index) {
    if (index < RECIPE_COUNT) {
        return &AVAILABLE_RECIPES[index];
    }
    return nullptr; // 索引无效
}

// 获取配方索引
int getRecipeIndex(const String& name) {
    for (uint8_t i = 0; i < RECIPE_COUNT; i++) {
        if (AVAILABLE_RECIPES[i].recipeName == name) {
            return i;
        }
    }
    return -1; // 未找到
}

#endif // RECIPE_H

