# ACard 项目结构与文件职责

本文是实现前的目录蓝图，不表示所有列出的文件已存在。按模块组织，头文件与源文件放在一起。文件随实现需要创建，不先生成空壳。整体行为见 ARCHITECTURE.md，玩法见 RULE.md。

## 1. 总体目录

```text
ACardGame/
├── CMakeLists.txt
├── CMakePresets.json
├── .gitignore
├── .gitattributes
├── cmake/
│   └── Dependencies.cmake
├── docs/
│   ├── RULE.md
│   ├── ARCHITECTURE.md
│   ├── PROJECT_STRUCTURE.md
│   ├── PROTOCOL.md
│   └── BUILD.md
├── server/
│   ├── CMakeLists.txt
│   ├── ids/
│   │   └── ids.h
│   ├── cards/
│   │   ├── card.h
│   │   ├── deck.h
│   │   └── deck.cpp
│   ├── game/
│   │   ├── game_types.h
│   │   ├── game_deck.h / game_deck.cpp
│   │   ├── hand_rules.h / hand_rules.cpp
│   │   ├── game_state.h
│   │   ├── game.h / game.cpp
│   │   ├── game_view.h / game_view.cpp
│   │   └── settlement.h / settlement.cpp
│   ├── room/
│   │   ├── room.h / room.cpp
│   │   └── room_manager.h / room_manager.cpp
│   ├── session/
│   │   └── session_manager.h / session_manager.cpp
│   ├── protocol/
│   │   ├── messages.h
│   │   └── message_codec.h / message_codec.cpp
│   ├── network/
│   │   ├── frame_codec.h / frame_codec.cpp
│   │   ├── tcp_connection.h / tcp_connection.cpp
│   │   └── tcp_server.h / tcp_server.cpp
│   ├── app/
│   │   ├── main.cpp
│   │   ├── server_app.h / server_app.cpp
│   │   ├── server_config.h / server_config.cpp
│   │   └── logging.h / logging.cpp
│   └── config/
│       └── server.example.json
├── tools/
│   ├── CMakeLists.txt
│   └── game_cli.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── cards_tests.cpp
│   ├── hand_rules_tests.cpp
│   ├── game_tests.cpp
│   ├── wind_tests.cpp
│   ├── settlement_tests.cpp
│   ├── room_tests.cpp
│   ├── protocol_tests.cpp
│   └── frame_tests.cpp
└── client/
    ├── project.godot
    ├── export_presets.cfg
    ├── app/
    │   ├── app.tscn / app.gd
    │   └── local_server.gd
    ├── network/
    │   ├── server_connection.gd
    │   └── frame_codec.gd
    ├── protocol/
    │   ├── messages.gd
    │   └── message_codec.gd
    ├── state/
    │   ├── client_state.gd
    │   └── game_actions.gd
    ├── ui/
    │   ├── menu.tscn / menu.gd
    │   ├── room.tscn / room.gd
    │   ├── table.tscn / table.gd
    │   ├── result.tscn / result.gd
    │   └── card.tscn / card.gd
    └── assets/
        ├── cards/
        ├── fonts/
        └── audio/
```

斜线并列表示一对文件，不是实际文件名。build/、dist/、logs/、Godot 的 .godot/ 为生成目录，不提交。现有 gptdocs/ 保留为历史材料，不作为当前规则和协议的权威来源。

## 2. 构建与文档

| 文件 | 职责 |
| --- | --- |
| 根 CMakeLists.txt | 定义 C++ 标准、测试开关，加入 server、tools、tests；不负责 Godot 导出 |
| CMakePresets.json | 提供 Windows/Linux 构建配置；不写个人绝对路径 |
| cmake/Dependencies.cmake | FetchContent 获取固定版本的 Asio、nlohmann/json、spdlog、Catch2；不跟踪最新分支 |
| server/CMakeLists.txt | 定义独立模块目标及依赖，避免全项目共享所有头文件目录 |
| docs/PROTOCOL.md | 消息边界、版本、卡牌编码、请求/响应/通知字段、错误、同步与心跳；已编写第一版规范 |
| docs/BUILD.md | 本机启动接口、跨平台构建与发布约束、验收矩阵；实际版本和命令随工程初始化补齐 |

## 3. 卡牌与规则核心

| 文件 | 职责 |
| --- | --- |
| cards/card.h | Suit、Rank、Card、相等比较；不判断本游戏大小，不存图片路径和网络编号 |
| cards/deck.h/.cpp | 接受牌列表和随机源进行洗牌；按起始位置逐张轮流分配；验证输入，不接触 PlayerId |
| game/game_types.h | 本局玩家位置、阶段、阵营、牌型描述、操作类型、规则错误与操作结果 |
| game/game_deck.h/.cpp | 生成 ACard 的 40 张牌，确定本游戏身份牌；不管理房间 |
| game/hand_rules.h/.cpp | 识别与比较牌型、身份牌优先检查、查询是否存在合法管牌组合；查询不得修改手牌 |
| game/game_state.h | 完整内部牌局数据；不包含 JSON 或连接，不向客户端直接发送 |
| game/game.h/.cpp | 对外牌局入口，持有状态；验证操作、推进回合与接风、记录名次、结束或中止 |
| game/game_view.h/.cpp | 个人可见数据结构及投影；本人手牌、公共信息、合法操作提示；隐藏他人手牌和未公开阵营 |
| game/settlement.h/.cpp | 根据人数、初始阵营和最终名次产生胜负，不按当前剩余身份牌重新划分阵营 |

game.cpp 先集中实现状态推进，不预先为每个阶段建立类。牌局只能通过受控接口修改，不能让房间直接改 GameState 字段。

## 4. 房间与会话

| 文件 | 职责 |
| --- | --- |
| room/room.h/.cpp | 单个房间、座位与房主、准备、开局条件、牌局所有权、上一局有效第一名；映射 PlayerId 与本局位置 |
| room/room_manager.h/.cpp | 持有全部房间，创建/查找/删除；不重复单房间规则 |
| session/session_manager.h/.cpp | 会话数据与管理，ConnectionId 和临时 PlayerId 的映射、所在房间；不持有牌局手牌 |

初版 Session 结构可直接放在 session_manager.h，不需要每个结构一个文件。ConnectionId、PlayerId、RoomId、GameId 使用 server/ids/ids.h 中独立的值类型，仅依赖标准库，不引入 Asio、JSON 或业务模块；acard_ids 为 header-only 目标。会话保存 ConnectionId 和可空 RoomId，不包含网络/房间头文件或对象指针。规则核心只使用本局位置，不依赖 acard_ids。

## 5. 网络、协议与应用

| 文件 | 职责 |
| --- | --- |
| network/frame_codec.h/.cpp | 4 字节网络字节序长度头；拼接拆包、长度上限、残留缓冲；不解析 JSON |
| network/tcp_connection.h/.cpp | 单个 socket 的异步读写、完整帧回调、发送队列、关闭；一次只能有一个写操作在进行 |
| network/tcp_server.h/.cpp | 监听、接受连接、管理连接、按 ConnectionId 发送和关闭；向应用报告消息和断线 |
| protocol/messages.h | C++ 请求/响应/通知数据结构，字段按协议定义；不含 socket |
| protocol/message_codec.h/.cpp | JSON 与消息对象转换、字段类型/范围检查、卡牌协议编码；不执行出牌规则 |
| app/server_app.h/.cpp | 持有并组装网络、会话、房间；握手与请求分发、身份校验、响应和状态广播、断线清理、心跳调度 |
| app/server_config.h/.cpp | 端口、地址、日志等配置与启动参数解析，检查配置是否合法 |
| app/logging.h/.cpp | 初始化 spdlog 控制台及文件输出；日志不得无意记录所有玩家秘密手牌 |
| app/main.cpp | 读取配置、初始化日志与应用、运行单线程事件循环、处理退出，不实现业务 |
| config/server.example.json | 可参考的默认配置，不包含机器私有路径 |

Asio 类型只在网络和应用运行调度中出现。协议层可依赖规则中的操作与视图类型，规则核心不能反向依赖协议或 JSON。房间产生业务结果，应用负责把结果送给哪些连接。

结算和中止时，应用必须取得并排队发送最终通知/视图，再释放活动牌局；不能先释放对象再读取结果。网络回调的对象生命期必须保证有效，关闭后不得再次执行业务。

### 所有权与异步关闭约定

- ServerApp 持有 TcpServer、SessionManager、RoomManager；RoomManager 独占各 Room，Room 独占活动 Game。会话只保存值标识，跨模块操作由应用查找，不互相持有所有权。
- TcpServer 管理连接，连接使用共享所有权以维持异步回调期间的生命期；每个读/写/定时器回调捕获该连接的共享引用，连接向应用报告事件的回调不拥有应用。应用存活至连接关闭及取消回调排空后。
- 连接关闭入口幂等：先标记 closing，取消计时器及 socket 操作，后续取消回调只做资源收尾，不分发帧、不推进业务。向应用报告断线最多一次；已排队写缓冲由连接持有至完成或取消，不能引用临时 JSON 字符串。
- 房间操作返回拥有自身数据的结果值，包含待通知成员标识、最终个人视图及结算/中止数据；不得把 GameState 引用或裸指针交给异步发送队列。应用按协议顺序编码并入队后，让房间释放 Game；真正网络发送完成不需要 Game 继续存活。
- 全局关闭由应用统一协调，先取得所有需要的最终数据，再入队关闭通知并开始发送宽限；房间离开路径不能重复中止已结束的牌局。宽限结束取消剩余操作，排空取消回调后退出事件循环。

## 6. Godot 客户端

| 文件 | 职责 |
| --- | --- |
| app/app.tscn/.gd | 应用根节点，组装连接、状态和界面，切换菜单/房间/牌桌/结果；不重新实现规则 |
| app/local_server.gd | 按 BUILD.md 定位程序与指定端口，启动并重试初始连接，完成所有权握手与入房后判定就绪；管理本机进程失败与回收 |
| network/server_connection.gd | TCP 连接与轮询、收发缓冲和发送队列、连接状态信号；不阻塞界面主线程 |
| network/frame_codec.gd | 与 C++ 相同的长度帧格式和限制，处理半包/多包 |
| protocol/messages.gd | 按 PROTOCOL.md 定义消息名称和客户端消息构造约定 |
| protocol/message_codec.gd | 严格 JSON/UTF-8/重复键/深度检查、编解码、整数适配及服务端消息结构检查 |
| state/client_state.gd | 保存身份、房间和个人牌局视图，处理 game_id/revision，发出状态变更信号 |
| state/game_actions.gd | 把界面意图转为请求，管理请求序号与待响应请求；不提前修改权威手牌 |
| ui/menu | 创建本机服务端、连接地址、创建/加入房间 |
| ui/room | 座位、准备、房主开始和离开 |
| ui/table | 手牌、公共出牌、行动者、接风和操作按钮；动画不阻塞状态接收 |
| ui/result | 名次、阵营、胜负或中止原因；结果确认只是本地界面行为，不阻止服务器回到等待阶段 |
| ui/card | 单张牌的显示、选择和视觉反馈 |

初版由根节点持有服务，不要求把每个模块都设为 Autoload。客户端不按暗牌推断并替代服务端判定。

## 7. CMake 目标与依赖

```text
acard_cards     → C++ 标准库
acard_ids       → C++ 标准库（header-only）
acard_game      → acard_cards
acard_room      → acard_game、acard_ids
acard_session   → acard_ids
acard_network   → Asio、acard_ids
acard_protocol  → acard_game、acard_room、acard_ids、nlohmann/json
acard_server    → 上述模块、spdlog、nlohmann/json（配置）
acard_cli       → acard_game
acard_tests     → 被测试模块、Catch2（不链接服务器 main）
```

初版模块使用静态库或 header-only 目标，不引入 DLL/插件 ABI。Catch2 仅测试需要，Godot 不参与 CMake 编译。网络异步收发由一个事件循环线程执行，业务也在该线程顺序处理，回调不得执行阻塞等待。

## 8. 测试与命令行工具

tools/game_cli.cpp 用固定种子或指定手牌模拟玩家操作，输出回合和结果，只用于开发，不作为正式服务端入口。

测试重点：牌数与唯一性、拆牌与牌型比较、身份牌优先、非法操作不改变状态、过牌与出完、接风拒绝的强制出牌、所有胜负组合、房主交接和中止、JSON 类型错误、半包/多包和超长帧。

通信格式与示例见 [PROTOCOL.md](PROTOCOL.md)，应使用同样的样例验证 C++ 与 GDScript 实现，避免两端字段或卡牌编号不一致。

## 9. 发布布局（与源码布局不同）

```text
dist/<平台>/ACardGame/
├── ACardGame.exe 或对应平台客户端
├── ACardGame.pck（可嵌入）
└── server/
    ├── ACardServer.exe 或对应平台服务端
    ├── server.json
    └── 必需运行库
```

独立服务端另行打包。程序定位、日志与可写配置路径、运行库及跨平台验收要求见 [BUILD.md](BUILD.md)，不假设安装目录可写。客户端运行时寻找发布目录中的服务端，不使用开发机器的绝对路径。

## 10. 协议规范

完整第一版规范见 [PROTOCOL.md](PROTOCOL.md)，已覆盖请求/响应/通知字段、卡牌编号、握手、关闭、心跳、长度上限、错误码、消息顺序、revision 和过期请求。启动参数见 [BUILD.md](BUILD.md)；工具与依赖库的固定版本在工程初始化时落实。

## 11. Git 文件与版本管理

一个 Git 仓库管理客户端、服务端、协议与文档，使一次协议修改和两端实现能放在同一次提交中。

| 文件或目录 | 用途 | 是否提交 |
| --- | --- | --- |
| .git/ | Git 自动维护的历史、索引与本地仓库配置，不是源码模块，不手写 | 不作为文件提交 |
| .gitignore | 排除构建产物、缓存、个人配置和凭据 | 是 |
| .gitattributes | 统一文本换行，声明二进制资源，减少 Windows/Linux 无意义差异 | 是 |
| CMakePresets.json | 团队共享的构建配置 | 是 |
| CMakeUserPresets.json | 个人工具链和机器路径 | 否 |
| client/project.godot、export_presets.cfg | Godot 项目及共享导出设置，不能写入密码 | 是 |
| client/**/*.uid、资源旁的 *.import | Godot 生成的稳定资源标识或导入设置（若存在），保持资源引用和导入配置一致 | 是 |
| client/.godot/ | 可重建的缓存、编辑器状态和导出凭据 | 否 |
| client/assets/ | 原始图片、字体、声音等游戏输入资源 | 是 |
| server/config/server.example.json | 共享配置样例 | 是 |
| server/config/server.local.json | 个人运行配置，程序须明确选择或读取此文件 | 否 |
| build/、out/、dist/、logs/ | 构建、发布和运行产物 | 否 |
| .github/workflows/（后续可选） | 若使用 GitHub，自动执行 Windows/Linux 构建与测试 | 建立后提交 |

忽略规则不删除文件，也不使已跟踪文件自动停止跟踪。本方案要求 CMake 在 build/ 或 out/ 内构建，FetchContent 的下载内容也留在构建目录，不提交第三方缓存。共享配置与私有配置的加载行为需要在配置模块实现，文件名本身不会自动实现覆盖。

不忽略所有 *.cfg、*.json、*.uid、*.import，也不忽略整个 assets/，以免漏掉重建项目所需的输入。当前采用静态库布局，暂不建立 .gitmodules；若以后实际使用子模块再增加。

### 提交与分支约定

- 初版保持一个仓库，main 保存可构建、可验证的进度；较大的功能使用 feature/<名称> 分支。
- 一次提交解决一个明确问题，相关实现、测试、协议与文档一起提交。
- 提交前检查差异、遗漏文件及对应测试；代码尚未完成时不将半成品合并到 main。
- 建议提交说明采用 feat:、fix:、test:、docs:、build: 前缀，后接具体变化；这是项目约定，不是 Git 强制要求。
- 已发布 Demo 用标签标记，例如 v0.1.0；dist/ 不进源码历史，发布包单独保存。
- 大型素材确实造成历史体积问题时再考虑 Git LFS，当前不预先引入。

当前已创建 .gitignore 和 .gitattributes。仓库初始化可在开始工程实现时进行；初始化仓库不代表自动配置远程或推送。

