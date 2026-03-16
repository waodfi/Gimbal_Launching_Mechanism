# Gimbal Launching Mechanism - 自动打靶与追瞄系统

这是一个集成了高精度双轴云台（双舵机跟踪）与自动闭环发弹装置（N20编码器闭环控制）的 Arduino 状态机实战项目。

该项目已彻底重构为**模块化结构**，各组件的职责隔离非常清晰，便于逐个硬件模块分别进行调试而不发生冲突。

## 目录结构与详细说明书

针对整个项目，目前拆分出了多个维度的说明书文档（存放在 `docs/` 目录中）。当你需要在开发和后续对机械结构适配参数调节时，请查阅对应模块的使用手册：

1. **[核心引脚指北 - README_Config.md](docs/README_Config.md)**  
   - 包含：如何调换硬件插线、更换舵机及摇杆管脚、查看外部硬中断极限引脚的使用限制等。

2. **[主干状态机说明 - README_Main.md](docs/README_Main.md)**  
   - 包含：比赛/调试模式怎么切换、大业务环的射击延时间隔等应该到哪里去微调。

3. **[云台与 EEPROM 记忆模块 - README_Gimbal_Module.md](docs/README_Gimbal_Module.md)**  
   - 包含：遥控摇杆的死区范围、舵机转向反了如何处理、断电重置内存点的数据擦除判定等。

4. **[发射与供弹闭环控制 - README_Launcher_Module.md](docs/README_Launcher_Module.md)**  
   - 包含：如果你发现**电机拉不到底/转得不够一圈/供弹槽位没对准60度**等最要命的位移问题，请必须来这里按说明书中提示排查和动态换算所需填入程序的脉冲数量（PULSES_PER_REV）！

> *Author: waodfi | GitHub: GIMBAL_LAUNCHING_MECHANISM*