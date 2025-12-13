# 自动滚冲机原理图设计

本目录包含使用 Python SKiDL 设计的自动滚冲机原理图。

## 文件说明

- `schematic.py` - SKiDL 原理图设计脚本
- `README.md` - 本说明文件

## 依赖安装

### 1. 安装 Python 3

确保系统已安装 Python 3.6 或更高版本。

### 2. 安装 SKiDL

```bash
pip install skidl
```

或者使用 requirements.txt：

```bash
pip install -r requirements.txt
```

### 3. 安装 KiCad（可选）

如果需要查看和编辑生成的原理图，需要安装 KiCad：

- macOS: `brew install kicad`
- Linux: `sudo apt-get install kicad` (Ubuntu/Debian)
- Windows: 从 [KiCad 官网](https://www.kicad.org/download/) 下载安装

## 使用方法

### 生成原理图

运行脚本生成原理图文件：

```bash
python schematic.py
```

脚本会生成以下文件：
- `dev_machine.net` - 网表文件（用于 PCB 设计）
- `dev_machine.xml` - XML 格式的原理图数据

### 在 KiCad 中查看

1. 打开 KiCad
2. 创建新项目或打开现有项目
3. 导入生成的网表文件，或使用 KiCad 的导入功能加载 XML 文件

## 硬件连接说明

### Arduino 引脚分配

| 引脚 | 功能 | 连接设备 |
|------|------|----------|
| D2   | 步进电机 STEP | A4988 STEP |
| D3   | 步进电机 DIR | A4988 DIR |
| D4   | 步进电机 ENABLE | A4988 ENABLE |
| D5   | 水阀继电器 | 继电器模块 |
| D6   | 显影液阀继电器 | 继电器模块 |
| D7   | 漂定液阀继电器 | 继电器模块 |
| D8   | 出液泵方向 | BTS7960 DIR |
| D9   | 出液泵 PWM | BTS7960 PWM |
| D10  | 定影液阀继电器 | 继电器模块 |
| D11  | 进液泵继电器 | 继电器模块 |
| D12  | 开始/暂停按钮 | 按钮（INPUT_PULLUP） |
| D13  | 选择配方按钮 | 按钮（INPUT_PULLUP） |
| A0   | 紧急停止按钮 | 按钮（INPUT_PULLUP） |
| SDA  | I2C 数据线 | LCD I2C SDA |
| SCL  | I2C 时钟线 | LCD I2C SCL |

### 主要硬件模块

1. **Arduino 主控板**
   - 使用 Arduino Uno 或兼容板
   - 5V 工作电压

2. **A4988 步进电机驱动**
   - 控制滚冲步进电机
   - 需要 5V 逻辑电源和 12V 电机电源

3. **BTS7960 出液泵驱动**
   - 控制出液泵（直流电机）
   - PWM 速度控制
   - 需要 5V 逻辑电源和 12V 电机电源

4. **5V 继电器模块（5个）**
   - 控制水阀、显影液阀、漂定液阀、定影液阀
   - 控制进液泵
   - 低电平触发

5. **2004 I2C LCD 显示模块**
   - 20列 x 4行 LCD
   - I2C 接口（地址通常为 0x27 或 0x3F）

6. **按钮输入**
   - 3个按钮：开始/暂停、选择配方、紧急停止
   - 使用 Arduino 内部上拉电阻（INPUT_PULLUP）

### 电源要求

- **5V 电源**：用于 Arduino、继电器模块、LCD、驱动模块逻辑部分
- **12V 电源**：用于步进电机、出液泵、进液泵、阀门

## 注意事项

1. **电源隔离**：确保 5V 和 12V 电源共地，但要注意电流容量
2. **继电器驱动**：代码中使用低电平触发，确保继电器模块支持
3. **I2C 上拉电阻**：LCD 模块通常内置上拉电阻，如无则需外接
4. **电机保护**：建议在电机电源线上添加保险丝或保护电路
5. **按钮防抖**：代码中已实现软件防抖，硬件上也可添加电容

## 故障排除

### SKiDL 找不到库

如果运行脚本时提示找不到库，可能需要：

1. 检查 KiCad 是否已安装
2. 设置 KiCad 库路径：
   ```python
   lib_search_paths[KICAD].append('/path/to/kicad/symbols')
   ```

### 生成的原理图不完整

某些符号可能无法找到，脚本会使用通用连接器替代。可以在 KiCad 中手动替换为正确的符号。

## 参考资料

- [SKiDL 文档](https://xess.com/skidl/docs/)
- [KiCad 文档](https://docs.kicad.org/)
- [Arduino 文档](https://www.arduino.cc/reference/)
