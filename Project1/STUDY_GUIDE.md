# Project 1 学习顺序

当前代码已经按职责拆分：

- `project1.cpp`：C++ 基础、命令行解析和 `main()`
- `treasure_hunt.hpp`：`enum`、`struct`、class 接口和成员变量
- `treasure_hunt.cpp`：地图、容器、搜索、路径回溯与输出算法

建议分 8 天阅读：

1. `project1.cpp` 1–43、132–143：头文件、函数、输入输出和 `main()`。
2. `treasure_hunt.hpp` 10–36，`project1.cpp` 45–63：enum、struct、引用、const。
3. `project1.cpp` 31–130：字符串、循环、switch 和 `getopt_long()`。
4. `treasure_hunt.hpp` 38–80，`treasure_hunt.cpp` 22–47：class 声明与成员函数实现。
5. `treasure_hunt.hpp` 32–58，`treasure_hunt.cpp` 24–47、124–182：Tile 与二维 vector 地图。
6. `treasure_hunt.cpp` 49–77、184–195、253–279：用 deque 实现 Stack/Queue。
7. `treasure_hunt.cpp` 49–108、197–289：邻接关系、discover 与两层搜索。
8. `treasure_hunt.cpp` 229–389：父节点方向、路径回溯和输出。

每天结束时尝试回答：数据存在哪里、哪个函数改变它、写错后输出会有什么变化。
