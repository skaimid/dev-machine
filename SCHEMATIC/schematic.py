#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动滚冲机原理图设计
使用 SKiDL 生成 KiCad 原理图

硬件连接说明：
- BTS7960 出液泵：PWM引脚9，方向引脚8
- A4988 步进电机（滚冲）：步进引脚2，方向引脚3，使能引脚4
- 5V继电器模块：
  * 水阀：引脚5
  * 显影液阀：引脚6
  * 漂定液阀：引脚7
  * 定影液阀：引脚10
  * 进液泵：引脚11
- 2004 I2C LCD：SDA/SCL（根据Arduino型号）
- 按钮：
  * 开始/暂停：引脚12
  * 选择配方：引脚13
  * 紧急停止：A0
"""

from skidl import *

# ============================================
# 创建电路网络
# ============================================

# 电源网络
gnd = Net('GND')
vcc_5v = Net('VCC_5V')
vcc_12v = Net('VCC_12V')  # 用于步进电机和泵

# Arduino 数字引脚网络
arduino_pin_2 = Net('ARDUINO_PIN_2')   # 步进电机 STEP
arduino_pin_3 = Net('ARDUINO_PIN_3')   # 步进电机 DIR
arduino_pin_4 = Net('ARDUINO_PIN_4')   # 步进电机 ENABLE
arduino_pin_5 = Net('ARDUINO_PIN_5')   # 水阀继电器
arduino_pin_6 = Net('ARDUINO_PIN_6')   # 显影液阀继电器
arduino_pin_7 = Net('ARDUINO_PIN_7')   # 漂定液阀继电器
arduino_pin_8 = Net('ARDUINO_PIN_8')   # 出液泵方向
arduino_pin_9 = Net('ARDUINO_PIN_9')   # 出液泵 PWM
arduino_pin_10 = Net('ARDUINO_PIN_10') # 定影液阀继电器
arduino_pin_11 = Net('ARDUINO_PIN_11') # 进液泵继电器
arduino_pin_12 = Net('ARDUINO_PIN_12') # 开始/暂停按钮
arduino_pin_13 = Net('ARDUINO_PIN_13') # 选择配方按钮
arduino_pin_a0 = Net('ARDUINO_PIN_A0') # 紧急停止按钮

# I2C 网络
i2c_sda = Net('I2C_SDA')
i2c_scl = Net('I2C_SCL')

# ============================================
# Arduino 主控板（使用通用符号）
# ============================================
# 注意：实际使用中，Arduino Uno 通常作为模块使用
# 这里使用连接器来表示 Arduino 的接口
arduino_connector = Part('Connector', 'Conn_01x16_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x16_P2.54mm_Vertical')
arduino_connector.ref = 'ARDUINO'

# Arduino 电源连接（假设引脚1=VCC，引脚2=GND）
arduino_connector[1] += vcc_5v
arduino_connector[2] += gnd

# Arduino 引脚连接
arduino_connector[3] += arduino_pin_2   # D2
arduino_connector[4] += arduino_pin_3   # D3
arduino_connector[5] += arduino_pin_4   # D4
arduino_connector[6] += arduino_pin_5   # D5
arduino_connector[7] += arduino_pin_6   # D6
arduino_connector[8] += arduino_pin_7   # D7
arduino_connector[9] += arduino_pin_8   # D8
arduino_connector[10] += arduino_pin_9  # D9
arduino_connector[11] += arduino_pin_10 # D10
arduino_connector[12] += arduino_pin_11 # D11
arduino_connector[13] += arduino_pin_12 # D12
arduino_connector[14] += arduino_pin_13 # D13
arduino_connector[15] += arduino_pin_a0 # A0

# I2C 连接（假设 A4=SDA, A5=SCL）
arduino_connector[16] += i2c_sda
arduino_connector[17] += i2c_scl

# ============================================
# A4988 步进电机驱动模块
# ============================================
# 使用通用连接器表示 A4988 模块
stepper_driver = Part('Connector', 'Conn_01x16_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x16_P2.54mm_Vertical')
stepper_driver.ref = 'A4988'

# A4988 连接
stepper_driver[1] += vcc_5v      # VDD
stepper_driver[2] += gnd          # GND
stepper_driver[3] += vcc_12v     # VMOT (电机电源)
stepper_driver[4] += arduino_pin_2  # STEP
stepper_driver[5] += arduino_pin_3  # DIR
stepper_driver[6] += arduino_pin_4  # ENABLE

# 步进电机输出（1A, 1B, 2A, 2B）
stepper_motor = Part('Connector', 'Conn_01x04_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical')
stepper_motor.ref = 'STEPPER_MOTOR'
stepper_driver[7] += stepper_motor[1]  # 1A
stepper_driver[8] += stepper_motor[2]  # 1B
stepper_driver[9] += stepper_motor[3]  # 2A
stepper_driver[10] += stepper_motor[4] # 2B

# ============================================
# BTS7960 出液泵驱动模块
# ============================================
# 使用通用连接器表示 BTS7960 模块
drain_pump_driver = Part('Connector', 'Conn_01x08_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical')
drain_pump_driver.ref = 'BTS7960'

# BTS7960 连接
drain_pump_driver[1] += vcc_5v       # VCC (逻辑电源)
drain_pump_driver[2] += gnd         # GND
drain_pump_driver[3] += vcc_12v     # VM (电机电源)
drain_pump_driver[4] += arduino_pin_9  # PWM
drain_pump_driver[5] += arduino_pin_8  # DIR

# 出液泵输出
drain_pump = Part('Connector', 'Conn_01x02_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
drain_pump.ref = 'DRAIN_PUMP'
drain_pump_driver[6] += drain_pump[1]  # OUT1
drain_pump_driver[7] += drain_pump[2]  # OUT2

# ============================================
# 5V 继电器模块（5个继电器）
# ============================================
# 使用通用继电器符号
relay_water = Part('Relay', 'Relay_SPDT', footprint='Relay_THT:Relay_DPDT_Schrack-RT2-FormA_RM5mm')
relay_water.ref = 'RLY1'
relay_water.value = 'Water Valve'

relay_dev = Part('Relay', 'Relay_SPDT', footprint='Relay_THT:Relay_DPDT_Schrack-RT2-FormA_RM5mm')
relay_dev.ref = 'RLY2'
relay_dev.value = 'Developer Valve'

relay_bleach = Part('Relay', 'Relay_SPDT', footprint='Relay_THT:Relay_DPDT_Schrack-RT2-FormA_RM5mm')
relay_bleach.ref = 'RLY3'
relay_bleach.value = 'Bleach Valve'

relay_fix = Part('Relay', 'Relay_SPDT', footprint='Relay_THT:Relay_DPDT_Schrack-RT2-FormA_RM5mm')
relay_fix.ref = 'RLY4'
relay_fix.value = 'Fixer Valve'

relay_fill_pump = Part('Relay', 'Relay_SPDT', footprint='Relay_THT:Relay_DPDT_Schrack-RT2-FormA_RM5mm')
relay_fill_pump.ref = 'RLY5'
relay_fill_pump.value = 'Fill Pump'

# 继电器线圈连接（假设低电平触发）
for relay, pin in [(relay_water, arduino_pin_5),
                   (relay_dev, arduino_pin_6),
                   (relay_bleach, arduino_pin_7),
                   (relay_fix, arduino_pin_10),
                   (relay_fill_pump, arduino_pin_11)]:
    try:
        relay['Coil+'] += pin
        relay['Coil-'] += gnd
        relay['VCC'] += vcc_5v
        relay['GND'] += gnd
    except:
        # 如果引脚名称不同，尝试其他方式
        relay[1] += pin
        relay[2] += gnd
        relay[3] += vcc_5v
        relay[4] += gnd

# 阀门和进液泵连接器（通过继电器控制）
valve_water = Part('Connector', 'Conn_01x02_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
valve_water.ref = 'VALVE_WATER'
valve_dev = Part('Connector', 'Conn_01x02_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
valve_dev.ref = 'VALVE_DEV'
valve_bleach = Part('Connector', 'Conn_01x02_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
valve_bleach.ref = 'VALVE_BLEACH'
valve_fix = Part('Connector', 'Conn_01x02_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
valve_fix.ref = 'VALVE_FIX'
fill_pump = Part('Connector', 'Conn_01x02_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
fill_pump.ref = 'FILL_PUMP'

# 继电器触点连接（常开触点）
try:
    relay_water['NO'] += valve_water[1]
    relay_water['COM'] += vcc_12v
    relay_dev['NO'] += valve_dev[1]
    relay_dev['COM'] += vcc_12v
    relay_bleach['NO'] += valve_bleach[1]
    relay_bleach['COM'] += vcc_12v
    relay_fix['NO'] += valve_fix[1]
    relay_fix['COM'] += vcc_12v
    relay_fill_pump['NO'] += fill_pump[1]
    relay_fill_pump['COM'] += vcc_12v
except:
    # 如果引脚名称不同，使用数字索引
    relay_water[5] += valve_water[1]
    relay_water[6] += vcc_12v
    relay_dev[5] += valve_dev[1]
    relay_dev[6] += vcc_12v
    relay_bleach[5] += valve_bleach[1]
    relay_bleach[6] += vcc_12v
    relay_fix[5] += valve_fix[1]
    relay_fix[6] += vcc_12v
    relay_fill_pump[5] += fill_pump[1]
    relay_fill_pump[6] += vcc_12v

# 所有负载的负极接地
valve_water[2] += gnd
valve_dev[2] += gnd
valve_bleach[2] += gnd
valve_fix[2] += gnd
fill_pump[2] += gnd

# ============================================
# 2004 I2C LCD 显示模块
# ============================================
# 使用连接器表示 LCD 模块
lcd = Part('Connector', 'Conn_01x04_Male', footprint='Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical')
lcd.ref = 'LCD2004'
lcd.value = 'LCD 2004 I2C'

# LCD 连接
lcd[1] += vcc_5v
lcd[2] += gnd
lcd[3] += i2c_sda
lcd[4] += i2c_scl

# ============================================
# 按钮输入
# ============================================
# 开始/暂停按钮
button_start = Part('Switch', 'SW_Push', footprint='Button_Switch_THT:SW_PUSH_6mm')
button_start.ref = 'BTN_START'
button_start[1] += arduino_pin_12
button_start[2] += gnd

# 选择配方按钮
button_select = Part('Switch', 'SW_Push', footprint='Button_Switch_THT:SW_PUSH_6mm')
button_select.ref = 'BTN_SELECT'
button_select[1] += arduino_pin_13
button_select[2] += gnd

# 紧急停止按钮
button_emergency = Part('Switch', 'SW_Push', footprint='Button_Switch_THT:SW_PUSH_6mm')
button_emergency.ref = 'BTN_EMERGENCY'
button_emergency[1] += arduino_pin_a0
button_emergency[2] += gnd

# 注意：Arduino 内部有上拉电阻，但可以在原理图中添加外部上拉电阻
# 这里省略，因为代码中使用 INPUT_PULLUP

# ============================================
# 电源部分
# ============================================
# 5V 电源输入
power_5v = Part('Power', 'USB_C', footprint='Connector_USB:USB_C_Receptacle_USB2.0')
power_5v.ref = 'PWR_5V'
try:
    power_5v['VBUS'] += vcc_5v
    power_5v['GND'] += gnd
except:
    power_5v[1] += vcc_5v
    power_5v[2] += gnd

# 12V 电源输入（用于电机和泵）
power_12v = Part('Power', 'DC_Jack', footprint='Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical')
power_12v.ref = 'PWR_12V'
power_12v[1] += vcc_12v
power_12v[2] += gnd

# 电源滤波电容
cap_5v = Part('Device', 'C', footprint='Capacitor_THT:C_Disc_D3.0mm_W1.6mm_P2.50mm')
cap_5v.ref = 'C1'
cap_5v.value = '100uF'
cap_5v[1] += vcc_5v
cap_5v[2] += gnd

cap_12v = Part('Device', 'C', footprint='Capacitor_THT:C_Disc_D3.0mm_W1.6mm_P2.50mm')
cap_12v.ref = 'C2'
cap_12v.value = '100uF'
cap_12v[1] += vcc_12v
cap_12v[2] += gnd

# ============================================
# 生成原理图
# ============================================
# 生成 KiCad 原理图文件
ERC()  # 运行电气规则检查

# 生成网表（用于 PCB 设计）
generate_netlist(file_='dev_machine.net')

# 生成 XML 格式（用于 KiCad）
generate_xml(file_='dev_machine.xml')

print("=" * 50)
print("原理图生成完成！")
print("=" * 50)
print("生成的文件：")
print("  - dev_machine.net (网表文件)")
print("  - dev_machine.xml (XML 格式)")
print("")
print("硬件连接总结：")
print("  Arduino 引脚分配：")
print("    D2  -> A4988 STEP")
print("    D3  -> A4988 DIR")
print("    D4  -> A4988 ENABLE")
print("    D5  -> 水阀继电器")
print("    D6  -> 显影液阀继电器")
print("    D7  -> 漂定液阀继电器")
print("    D8  -> BTS7960 DIR")
print("    D9  -> BTS7960 PWM")
print("    D10 -> 定影液阀继电器")
print("    D11 -> 进液泵继电器")
print("    D12 -> 开始/暂停按钮")
print("    D13 -> 选择配方按钮")
print("    A0  -> 紧急停止按钮")
print("    SDA -> LCD I2C SDA")
print("    SCL -> LCD I2C SCL")
print("=" * 50)
