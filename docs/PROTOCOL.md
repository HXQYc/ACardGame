# ACard 通信协议 v1

状态：第一版实现规范。既有需求已纳入；本文新增的具体编码、超时、限制和启动控制方式为设计选定值，可在实现前调整。服务端与客户端必须同时遵守本文，不自行采用不同默认值。

适用范围：C++ 独立服务端、Godot/GDScript 客户端、Windows/Linux、三人或四人、局域网联机。初版每个服务器暴露一个默认房间，内部可管理多房间，但不提供房间列表。无账号、重连、观战、AI、进贡与明牌加倍。

## 1. 基本原则

- 服务器是唯一权威；客户端只提交意图。
- 身份从连接绑定的会话确定，不接受客户端指定行动者。
- 每次成功状态操作后发送完整个人可见状态；响应不用于自行推演牌局。
- TCP 内按顺序处理和发送；同一连接只有一个正在执行的异步写操作。
- 规则操作失败时不改变牌局状态。
- JSON 不直接序列化完整内部 GameState，必须先生成个人视图。
- 昵称是展示信息，可重复；PlayerId 是服务端临时分配的身份。

## 2. 传输帧

```text
4 字节 uint32 大端长度 N | N 字节 UTF-8 JSON
```

- 长度只计 JSON 字节，不含长度头；不得按字符数计算。
- `1 <= N <= 65536`。零长度和超长帧立即关闭连接。
- JSON 必须是单个对象，不能附加另一段 JSON；不含 BOM。
- 接收缓冲支持拆包和多包：先收齐长度头，再收齐正文，再处理下一帧。
- UTF-8 无效、JSON 语法错误、重复对象键或非对象根节点属于格式错误。
- 不能用“读一次 socket”代表“读到一条消息”。
- 长度头或正文收到第一个字节起，10 秒内必须完成整帧，否则关闭；空闲但未开始帧由心跳控制。
- 每个连接待发送队列（包括正在发送帧）最多 256 KiB，超过时关闭慢连接，不静默丢弃牌局更新。
- 解析前限制 JSON 嵌套深度为 16；具体实现应防止深层 JSON 耗尽资源。

两端的 message_codec 负责严格输入校验：UTF-8 必须有效，字符串转义中的代理项必须正确配对；拒绝尾随逗号、字符串内未转义的控制字符、重复键和非 JSON 数字。根对象深度为 1，每进入一个对象或数组加一，空容器也计一层，未知字段也受限制。先在有界帧上进行理解字符串及转义的语法/深度/重复键检查，再使用 JSON 库解码；不得仅凭库解析成功认定输入符合协议，也不得用简单正则代替检查。格式错误按第3节关闭流程处理。

重复键按解码后的键字符串比较，例如 `"a"` 与 `"\u0061"` 是同一键。整数的数学整数性和范围按原始十进制数字 token 精确检查，不能因浮点解析舍入而把 `7.0000000000000001` 接受为 7；检查时不得对任意长度的指数进行无界计算。库解码结果中的数值类型检查和转换是适配步骤，不能代替此输入检查。

## 3. 统一信封

### 请求

```json
{"version":1,"kind":"request","name":"room.ready","request_id":3,"payload":{"ready":true,"expected_room_revision":2}}
```

| 字段 | 类型 | 约束 |
| --- | --- | --- |
| version | 整数 | 本版为 1 |
| kind | 字符串 | request / response / event |
| name | 字符串 | 本文规定的消息名 |
| request_id | 整数 | 请求与响应必有；范围 1～2147483647 |
| payload | 对象 | 即使为空也写 `{}` |

客户端每条连接从 request_id=1 开始，严格递增，不要求连续，心跳也使用序号。达到上限后重新连接。服务端不缓存重复请求：重复或倒退的序号返回 REQUEST_ID_NOT_INCREASING，不执行。客户端不得因响应超时自动重发变更请求。

### 成功响应

```json
{"version":1,"kind":"response","name":"room.ready","request_id":3,"ok":true,"payload":{}}
```

### 失败响应

```json
{"version":1,"kind":"response","name":"game.play","request_id":8,"ok":false,"payload":{},"error":{"code":"NOT_YOUR_TURN","message":"当前不轮到你操作"}}
```

- response 的 name/request_id 与请求相同。
- 成功时没有 error；失败时必须有 error。
- code 是程序判断依据，message 是可显示的说明，不能用 message 做逻辑判断。
- 每个被接受为可解析请求的消息仅产生一个响应；随后可有状态通知。
- event 不含 request_id 和 ok。
- JSON 字段顺序无意义。忽略未知字段，但必须验证已知字段类型、必填项和范围；未知消息名返回 UNKNOWN_MESSAGE。
- 整数字段按数值判断：必须是有限 JSON number、数学上的整数且在字段范围内；允许 `7`、`7.0`、`7e0`，拒绝 `7.5`、字符串和布尔值。编码端统一输出不带小数和指数的整数形式。C++ 不得仅以 is_number_integer 判定合法性；GDScript 不得要求 JSON 解码结果必须为 TYPE_INT，也不得先强转整数再检查。两端先检查数值类型、有限性、范围和整数性，再转换为整数。本文所有整数最大不超过 2147483647，可由两端精确表示。
- ID 为非空 ASCII 字符串，最大 64 字节，客户端只保存、比较，不解释内部格式。

工具适配依据：Godot JSON 数字使用浮点解析，且默认解析器对部分非法语法/Unicode 较宽松；nlohmann/json 的 is_number_integer 判断其内部数值类型。上述协议约束高于库默认行为，具体工具版本在 BUILD.md 中固定。见 [Godot JSON 官方文档](https://docs.godotengine.org/en/4.5/classes/class_json.html)与 [nlohmann/json 整数检查官方文档](https://json.nlohmann.me/api/basic_json/is_number_integer/)。

结构严重损坏、无合法 name/request_id 或客户端发送 response/event 时，不生成不可靠的响应，发送 connection.closing（若连接可写）后关闭。

## 4. 标识、座位、卡牌编码

- player_id：握手成功后分配，断开后失效；服务器进程内不复用。
- room_id：默认房间标识，空房间保留到服务器退出，方便后续自动加入。
- game_id：每次开局分配的新标识，服务器进程内不复用，区分旧局请求。
- seat：房间固定座位 0～3，空位不占用 players 数组项。
- position：本局固定行动位置 0～人数减一；按占用座位升序生成，环形行动。
- 不因玩家出完牌重编号，本局始终保留参赛者列表。

### CardId

花色索引：梅花=0，方块=1，红桃=2，黑桃=3。

点数索引：5=0，6=1，7=2，8=3，9=4，10=5，J=6，Q=7，K=8，A=9。

`card_id = suit_index * 10 + rank_index`，合法范围 0～39。

例：梅花 A=9，红桃 5=20，红桃 K=28，黑桃 K=38，黑桃 A=39。

数组中顺序不决定牌型；同次请求不能重复 CardId。三人局梅花 A 仍为 9，但不具有身份牌优先级。

## 5. 建立会话与自动入房

连接状态：connected_unverified → authenticated → in_room。握手成功不等于入房成功。

### session.hello

客户端建立 TCP 后发送第一条请求：

```json
{"version":1,"kind":"request","name":"session.hello","request_id":1,"payload":{"nickname":"小明"}}
```

payload：nickname 必填；host_token 可选，仅本机服务器启动者使用。

昵称处理采用以下第一版设计值：只裁剪首尾 U+0020（普通空格），不执行 NFC/NFKC、大小写转换或其他替换；裁剪后必须为 1～20 个 Unicode 码点且 UTF-8 最多 80 字节。禁止 U+0000～U+001F、U+007F～U+009F；其他有效 Unicode 标量值保留，包括全角空格、组合字符和零宽字符。码点不是 UTF-8 字节、UTF-16 单元或视觉字形。服务端返回实际昵称，客户端预检查仅供提示，不要求昵称唯一。

成功 payload：

```json
{"player_id":"p_001","nickname":"小明","default_room_id":"r_default","heartbeat_interval_ms":5000,"connection_timeout_ms":20000,"is_server_owner":false}
```

- TCP 接受后 5 秒内必须收到 hello。
- 仅允许握手一次；重复返回 ALREADY_AUTHENTICATED。
- 未握手不得执行其他请求。
- 不兼容版本返回 VERSION_UNSUPPORTED，随后关闭。
- 昵称无效、host_token 无效返回对应错误，随后关闭。

### room.join_default

客户端 hello 成功后自动请求，不要求用户填写房间 ID：

```json
{"version":1,"kind":"request","name":"room.join_default","request_id":2,"payload":{}}
```

成功 payload：`{"room_id":"r_default","seat":0}`，随后向全部房间成员发送 room.state。

加入者分配最小空座位。空房间首位加入者为房主。房间游戏中或已满时返回错误。重复加入返回 ALREADY_IN_ROOM，不改变状态。

客户端入房失败后显示原因并断开；hello 后 10 秒内未成功入房，服务端关闭。初版没有大厅停留状态。

### 本机创建者优先与关闭权限

客户端创建房间时生成 32 字节密码学随机值，编码为 64 个小写十六进制字符，作为一次启动的 host_token。通过受控启动参数交给服务端，客户端在 hello 中提供相同值。不得通过昵称、IP 或第一个网络连接判断服务器所有者。

- 本机托管启动：服务端建立默认空房间，但在所有者入房前拒绝其他玩家，返回 HOST_NOT_READY。所有者成功入房后开放。
- 所有者 token 只能绑定一次会话，后续提供同值返回 HOST_TOKEN_USED；token 不广播、不写日志，不提交配置仓库。
- 服务端所有者绑定独立于房间房主，只有所有者可请求 server.shutdown。
- 独立服务器不设置 host_token，没有远程关闭权限；默认房间直接开放，首位加入者为房主。
- 从服务端成功完成监听并可接受连接起计时，所有者 15 秒内未入房，或已绑定所有者连接断开，本机托管服务器自动关闭。
- 所有者离开房间按初版约定会断开，因此本机托管服务器也结束。独立服务器上房主离开则仅转移房主权。

启动控制参数不是网络协议字段，固定端口、参数名称、就绪检测及进程回收见 [BUILD.md](BUILD.md)。局域网 v1 使用明文 TCP；token 用于所有权鉴别，不等于加密通道，不用于不可信公网部署。

## 6. 房间协议

### room.ready

payload：`{"ready":true,"expected_room_revision":2}`。ready 为布尔值；expected_room_revision 必须为 0～2147483647 的整数，room.start 同样适用。

成功 payload：`{}`。只能等待阶段执行；设置为现有值属于成功，不增加 revision，但在响应后只向请求者补发当前 room.state，不向其他成员广播。确有变化时响应后向全体成员广播新 room.state。版本检查仍先执行。

### room.start

payload：`{"expected_room_revision":5}`。

仅房主允许；房间等待中、人数为 3 或 4、所有人（包括房主）均已准备时开局。开局名单等于当前房间成员。

成功 payload：`{"game_id":"g_001"}`。发送顺序：请求者成功响应 → 全体 room.state（playing）→ 每人 game.state（revision=1）。

### room.leave

payload：`{}`。成功后回复，再通知其余成员，并向离开者发送 connection.closing，正常关闭连接。

成功响应 payload 为 `{}`。

普通成员正在游戏时离开：先中止牌局；响应后对其余成员发送 game.aborted，再发送等待状态 room.state。离开者不再作为房间广播对象。本机服务器所有者离开优先执行第11节统一服务器关闭事务，不先执行普通成员离开/房主交接/等待状态广播。

### room.state（event）

```json
{"version":1,"kind":"event","name":"room.state","payload":{"room_id":"r_default","revision":3,"host_player_id":"p_001","status":"waiting","game_id":null,"players":[{"player_id":"p_001","nickname":"小明","seat":0,"ready":true},{"player_id":"p_002","nickname":"小明","seat":1,"ready":false}]}}
```

- players 按 seat 升序，最多 4 项；昵称可重复。
- status 为 waiting/playing；playing 必须有 game_id，waiting 为 null。
- room revision 初始为 0，加入/离开/准备变化/开局/结束产生状态变化时加一；一次事务即使改变多个字段也只加一。
- revision 使用非负整数，最大 2147483647；达到上限时结束服务，不回绕复用。游戏 revision 同样不回绕。
- 牌局中的普通出牌不增加 room revision。
- 房主离开后，剩余最小 seat 成员接任；无成员时 host_player_id=null。
- 玩家加入或离开等待房间时，清除所有准备状态，要求按新名单重新准备。
- 游戏结束或中止，清除所有准备状态；结算结果独立保存，不嵌入每次 room.state。

## 7. 牌局请求

所有 game.* 请求共同必填：game_id（字符串）、expected_revision（整数，范围 1～2147483647）。

服务器先验证会话和所在房间，再验证 game_id、revision、阶段/行动者和具体规则。不能提交 player_id/position 来代替连接身份。

| 请求 | 额外 payload 字段 | 说明 |
| --- | --- | --- |
| game.play | cards：1～40 个 CardId | 统一处理领出、管牌及强制管牌 |
| game.pass | 无 | 自由领出和强制管牌时禁止 |
| game.request_wind | 无 | 仅接风选择阶段允许 |
| game.answer_wind | agree：布尔 | 仅当前被询问者允许；拒绝必须存在合法管牌组合 |

```json
{"version":1,"kind":"request","name":"game.play","request_id":8,"payload":{"game_id":"g_001","expected_revision":6,"cards":[28,38]}}
```

成功响应 payload：`{"game_id":"g_001","revision":7}`。随后向每位参赛者发送属于自己的 game.state。失败不增加 revision，除过期版本外不需重复广播状态。

牌型、身份牌优先、接风强制管牌等按 RULE.md 执行。错误不得包含他人的隐藏牌或未公开身份。

## 8. 个人牌局状态 game.state（event）

下面是格式示例，不代表固定牌局：

```json
{
  "version":1,
  "kind":"event",
  "name":"game.state",
  "payload":{
    "room_id":"r_default",
    "game_id":"g_001",
    "revision":7,
    "phase":"playing",
    "you_position":0,
    "your_team":"red",
    "your_cards":[0,10,20],
    "participants":[
      {"player_id":"p_001","nickname":"甲","seat":0,"position":0,"hand_count":3,"public_team":"unknown","finish_rank":null},
      {"player_id":"p_002","nickname":"乙","seat":1,"position":1,"hand_count":2,"public_team":"black","finish_rank":null},
      {"player_id":"p_003","nickname":"丙","seat":2,"position":2,"hand_count":4,"public_team":"unknown","finish_rank":null}
    ],
    "actor_position":1,
    "free_lead":false,
    "table_play":{"position":2,"cards":[18],"pattern":"single","card_count":1,"top_rank":"K"},
    "passed_positions":[],
    "wind":null,
    "allowed_actions":[],
    "last_action":{"position":2,"type":"play","cards":[18]},
    "change_reason":"play"
  }
}
```

字段约定：

| 字段 | 定义 |
| --- | --- |
| phase | playing / wind_choice / wind_poll / wind_resolution / finished |
| your_team | red/black，本人的真实阵营；不得包含其他人的真实阵营 |
| your_cards | 本人剩余牌 CardId 数组，按编号升序；可为空 |
| participants | 固定参赛列表，按 position 升序；3 或 4 项 |
| public_team | unknown/black；普通游戏中不主动公开红方，结束时变为 red/black |
| finish_rank | null 或 1～人数；出完立即赋值，最后一人结算时自动赋末名 |
| actor_position | 当前唯一可操作位置；finished 时 null |
| free_lead | 是否无管牌目标；free_lead=true 时 table_play=null，只有 play 可用 |
| table_play | 当前必须管住的牌；牌型及点数均来自服务端 |
| passed_positions | 自当前 table_play 建立以来已过牌的位置，按行动顺序；出新牌或自由领出时清空 |
| wind | 无接风上下文时 null，结构见下文 |
| allowed_actions | 对接收者有效的操作名数组，不是全体玩家权限 |
| last_action | 最近有效玩家操作；初始化时 null；用于动画而非重建状态 |
| change_reason | start/play/pass/request_wind/answer_wind/finished |

pattern：single/pair/straight/consecutive_pairs/bomb/thunder；top_rank：5～10 的字符串或 J/Q/K/A。card_count 必须等于 cards 长度。

allowed_actions：play/pass/request_wind/agree_wind/refuse_wind。非行动者数组为空。行动者仅在存在合法出牌组合时获得 play；不能管牌者不应显示可用管牌按钮。refuse_wind 必须经过存在合法管牌组合的查询。服务器仍需验证每条实际请求，不能因为曾下发允许操作就跳过验证。

last_action 按类型含字段：

- play：position、type="play"、cards。
- pass：position、type="pass"。
- request_wind：position、type="request_wind"。
- answer_wind：position、type="answer_wind"、agree。

不包含自己的 request_id，避免广播不同客户端的请求序号。客户端以 game_id+revision 去重动画，同 revision 的重新同步不重复播放。

### 接风上下文

```json
{"applicant_position":1,"finished_position":0,"poll_order":[2,3],"answers":[{"position":2,"agree":true}],"refuser_position":null,"step":"polling"}
```

- poll_order 为实际轮询顺序，不含已出完者和申请者。
- answers 为已发生的公开回答，按回答顺序。
- step：choice/following_passes/polling/applicant_decision/forced_play。
- wind 非 null 时示例中的全部字段必有。applicant_position 表示固定接风者 B，即使尚未申请；finished_position 为打出当前目标并出完的 A。refuser_position 无拒绝者时为 null。
- 接风选择阶段即存在 wind，step=choice，answers=[]、poll_order=[]、refuser_position=null。
- B 在 choice 直接过牌（未申请）：改为 phase=playing、free_lead=false、step=following_passes，保留 A 的 table_play 与 B 的 applicant_position；B 记入 passed_positions，行动者改为 B 后下一名仍有牌者。后续玩家仅可管牌或过牌，不能 request_wind；poll_order 和 answers 仍为空。
- following_passes 中全部剩余玩家过牌：B 获得自由领出，phase=playing、actor_position=B、free_lead=true、table_play=null、wind=null，清空 passed_positions。此转换随最后一次 pass 完成，不额外增加 revision。
- choice 或 following_passes 中有人管牌：清除旧 wind，清空 passed_positions，改用新 table_play；出牌者仍有牌则进入普通 playing，若其出完则创建以该玩家为 finished_position 的新接风上下文。只剩一人有牌时优先结算。
- B 申请时改为 wind_poll、step=polling，建立排除 B 和已出完者的 poll_order，actor_position 为首位被询问者；每次同意推进询问，第一个拒绝改为 wind_resolution/applicant_decision。
- wind_poll 时 actor_position 是当前被询问者。
- wind_resolution/applicant_decision 时 actor_position 是申请者。
- wind_resolution/forced_play 时 actor_position 是拒绝者。
- 第一个拒绝后不再询问后续玩家。
- 拒绝后申请者管牌，按上述管牌转换处理，拒绝者义务取消；申请者过牌则跳过已同意者，保持目标 A，进入 wind_resolution/forced_play，由拒绝者强制管牌。拒绝者管牌后也按上述管牌转换处理。
- 全部同意后恢复 playing、actor_position=B、free_lead=true、table_play=null、wind=null，清空 passed_positions；最后一次回答仍在 last_action 中公开。
- 中间已同意者被跳过不算真实提交 pass，不虚构他们的操作事件。

## 9. 结算与中止

### 正常结算

最后一次出牌触发结束时：成功响应 → 最终 game.state（phase=finished）→ game.finished → room.state（waiting）。每位客户端在其 TCP 上看到这个顺序。

最终 game.state：所有阵营公开、名次完整、actor_position=null、allowed_actions=[]、wind=null；仍只发送本人剩余手牌，末名玩家的其他手牌不公开。free_lead=false，保留最后一次 table_play 供展示。

game.finished payload：

```json
{"room_id":"r_default","game_id":"g_001","revision":18,"result":"black_win","rankings":[{"position":1,"player_id":"p_002","rank":1,"team":"black"},{"position":0,"player_id":"p_001","rank":2,"team":"red"},{"position":2,"player_id":"p_003","rank":3,"team":"red"}]}
```

result：black_win/red_win/draw。rankings 按 rank 升序。房间保存最后一次有效结算和第一名，随后释放活动牌局。

### 中止 game.aborted

```json
{"room_id":"r_default","game_id":"g_001","revision":9,"reason":"player_disconnected","departed_player_id":"p_002"}
```

- reason：player_disconnected/player_left/server_shutdown。
- 非玩家离开原因时 departed_player_id=null。
- 中止使游戏 revision 增加一，通知后不接受该局操作。
- 不生成胜负，不公开隐藏阵营或其他人的手牌，不覆盖上一局有效结算与第一名。
- 普通成员离开顺序为离开请求者响应（如存在）→ 其余参赛者 game.aborted → room.state（waiting）。所有者离开或断线优先执行统一关闭流程，reason=server_shutdown、departed_player_id=null，不再生成 player_left/player_disconnected 中止或等待状态。
- 客户端保留终止通知用于结果/中止界面；房间快照到达不得立即把结果弹窗清掉。

## 10. 版本序号与重新同步

- game revision 从 1 开始；每次有效游戏操作加一，结束不会额外增加第二次，最后一次操作与最终结算使用同一 revision。
- room revision 与 game revision 相互独立。
- 同一房间或牌局最多一个变更请求在途，心跳不受此限制。room.ready、room.start 和有效 game.* 的成功响应后，还须收到对应状态通知才能结束等待；无变化的 room.ready 使用同 revision 快照。响应不会自行修改权威状态。room.leave/server.shutdown 成功后进入关闭等待，不重新开放变更按钮。
- 普通业务失败在响应到达后结束等待；STALE_ROOM_STATE/STALE_GAME_STATE/GAME_NOT_ACTIVE 则须等待其约定的补发状态。断线或关闭统一清理所有等待。需要补发状态的响应到达后 10 秒未收到状态，客户端断开并提示同步异常，不自动重发。
- expected_revision 不匹配：返回 STALE_GAME_STATE，随后只给请求者发送当前个人 game.state，不执行操作。
- expected_room_revision 不匹配：返回 STALE_ROOM_STATE，随后只给请求者发送当前 room.state。
- 游戏 ID 不匹配：返回 GAME_MISMATCH，不把当前局当旧局执行。
- 不在活动牌局：返回 GAME_NOT_ACTIVE，附发当前 room.state。
- 服务端串行处理请求；迟到消息按处理时状态验证。
- 客户端同一 game_id 只接受更大 revision，等于则允许校正快照但不重复动画，更小则忽略。新 game_id 必须由有效开局状态引入，不能把迟到旧局切回当前局。
- 房间状态中 game_id 为 null 不等于删除本地结算信息。
- 同一 room_id 的快照只接受更大 revision；相等可校正并完成请求等待，更小忽略。服务端有序发送保证响应后的补发快照可用于结束等待；先于响应收到的无关状态不得提前完成该请求。

v1 不提供重连或独立 sync 请求；过期响应会自动附带当前状态。超时操作结果不确定时不重发，客户端断开并提示连接异常。

## 11. 心跳、限制与关闭

### connection.ping

已握手客户端每 5 秒发送 payload={}，服务端响应 payload={}。服务端无需依赖客户端时钟。

- 服务端 20 秒未收到合法请求时关闭连接；客户端 20 秒未收到合法服务端消息时断开。
- 非心跳请求 10 秒无对应响应，客户端提示连接异常并断开。
- 任意合法流量更新连接活跃时间，垃圾字节不续期。
- 默认请求限速每连接 token bucket：容量 40、每秒补充 20；超限响应 RATE_LIMITED，连续 5 次超限关闭，接受一次未超限请求后清零连续计数。
- 限速在请求序号检查之后执行；被限速请求仍消费 request_id，不改变业务状态。
- 初版不设玩家回合时限或 AI 托管；连接正常但不操作可一直等待，未来另行设计。

### server.shutdown

仅本机托管所有者允许，payload={}。房间房主身份不足以获得该权限。

成功响应 payload 为 `{}`。

成功响应后停止接受连接，活动牌局发 game.aborted（server_shutdown），对所有连接发 connection.closing，最多等待 1 秒发送完成后关闭并退出。独立服务器由操作者本地停止，不提供远程关闭。

统一关闭事务用于 server.shutdown、所有者 room.leave、已绑定所有者断线、所有者入房超时和独立服务端本地停止。首次进入即标记 shutting_down，停止接受新连接和业务请求，取消业务定时器；后续退出/断线回调不得重新进入事务。若由有效请求触发，先排队该请求的成功响应；每个活动牌局最多中止一次，revision 只加一次，发送 game.aborted（reason=server_shutdown、departed_player_id=null），再向仍可写连接发送 connection.closing（SERVER_SHUTDOWN）。所有者主动离房也遵守此顺序，不另发 CLIENT_LEFT；已断开的连接不发送。关闭时不执行房主交接或发送 waiting 房间快照。此前已结算/中止的牌局不得再次中止。1 秒发送宽限从进入关闭事务起计时，不阻塞事件循环。

### connection.closing（event）

payload：`{"code":"SERVER_SHUTDOWN","message":"服务器已关闭"}`。

code：CLIENT_LEFT/SERVER_SHUTDOWN/PROTOCOL_ERROR/HANDSHAKE_TIMEOUT/JOIN_TIMEOUT/HEARTBEAT_TIMEOUT/FRAME_TIMEOUT/SLOW_CLIENT/RATE_LIMITED。

通知尽力发送，非法长度、网络故障或不可写连接可直接关闭。客户端不能依赖收到此通知才能识别断线。断线回调重复触发也只清理一次会话和房间成员。

## 12. 错误码

| 错误码 | 含义 |
| --- | --- |
| VERSION_UNSUPPORTED | 协议版本不支持，关闭 |
| INVALID_MESSAGE | 信封不合法，关闭 |
| INVALID_ARGUMENT | 字段缺失、类型或范围不合法 |
| UNKNOWN_MESSAGE | 未定义请求名 |
| REQUEST_ID_NOT_INCREASING | 请求编号重复或倒退 |
| NOT_AUTHENTICATED | 尚未握手，关闭 |
| ALREADY_AUTHENTICATED | 重复握手 |
| INVALID_NICKNAME | 昵称不合法，关闭 |
| INVALID_HOST_TOKEN / HOST_TOKEN_USED | 本机所有权凭据错误或已用，关闭 |
| HOST_NOT_READY | 本机房主尚未入房 |
| ROOM_FULL / ROOM_IN_GAME | 默认房间已满或正在游戏 |
| ALREADY_IN_ROOM / NOT_IN_ROOM | 会话房间状态不符合操作 |
| NOT_ROOM_HOST | 只有房主可以开始 |
| INVALID_PLAYER_COUNT | 人数不是 3 或 4 |
| PLAYERS_NOT_READY | 不是所有人都准备好 |
| STALE_ROOM_STATE / STALE_GAME_STATE | 基于旧状态操作，附发新状态 |
| GAME_NOT_ACTIVE / GAME_MISMATCH | 没有活动游戏或指定了其他局 |
| NOT_YOUR_TURN | 当前不是该玩家操作 |
| ACTION_NOT_ALLOWED | 当前阶段不允许此操作 |
| DUPLICATE_CARD | 出牌数组有重复 |
| CARD_NOT_OWNED | 牌不在本人手里 |
| INVALID_PATTERN | 非法牌型 |
| IDENTITY_CARD_REQUIRED | 不符合身份牌 A 优先 |
| CANNOT_BEAT | 不能管住目标 |
| PASS_FORBIDDEN | 自由领出或强制管牌禁止过牌 |
| CANNOT_REFUSE_WIND | 没有合法管牌组合，不能拒绝 |
| NOT_SERVER_OWNER | 无权关闭服务器 |
| RATE_LIMITED | 请求频率过高 |
| INTERNAL_ERROR | 非预期内部错误，不泄漏实现或秘密信息 |

业务验证次序固定：参数结构 → 会话权限 → 房间/牌局身份 → 版本 → 行动者/阶段 → 卡牌重复与归属 → 牌型 → 身份牌优先 → 大小。内部异常导致状态可靠性无法保证时，中止服务器服务并记诊断日志，不继续把损坏状态当正常状态广播。

## 13. 客户端 UI 对应

- 创建：昵称 → 启动本机服务器 → 本机 hello（含 token）→ 自动入房 → 展示局域网 IP 与实际端口。
- 加入：昵称、IP、端口 → TCP → hello → join_default → room.state 后进入房间。
- 初版不显示可玩的单机入口。
- 全部准备后房主按钮可用，但最终仍由服务器验证。
- 等待变更响应/状态时禁用对应操作；连接、握手、入房各自显示进度和失败原因。
- 全量状态可立即应用；动画独立排队，不暂停网络处理。
- 昵称作为文本显示，不作为富文本标记或脚本执行。
- 服务器关闭或断线后退出活动房间，清理待响应请求与行动按钮，显示原因。

## 14. 实现验收

1. C++ 与 GDScript 使用相同帧和 CardId 编码，通过半包/多包/非 ASCII 昵称样例。
2. 每个请求有且仅有一个对应响应，未知请求、无效参数和错误状态能稳定返回错误。
3. 同一有效操作各玩家 revision 相同，但手牌字段只属于接收者。
4. 接风询问公开，拒绝者义务、跳过已同意者和重新进入普通回合可完整重放。
5. 最后一手结束、断线中止、房主交接、所有者退出不会读取已释放牌局或重复通知。
6. 重复请求、旧 game_id、过期 revision 不重复扣牌，不影响新局。
7. 本机所有者优先、关闭权限与默认房间自动加入一致；独立服务端不会被房间房主远程关闭。
8. 帧大小、深度、请求限速和慢连接上限能实际执行，恶意输入不泄漏隐藏信息。
9. 重复设置 ready 得到成功响应和同 revision 的请求者快照，客户端不永久等待；普通失败、过期失败和缺失后续快照均能结束或超时清理等待。
10. B 不申请直接过牌后，C/D 只能管牌或过牌；全过牌把领出权交给 B，有人管牌取消旧接风，有人管牌出完建立新接风，均有完整快照。
11. 对整数字段，两端接受 7/7.0/7e0，拒绝 7.5/7.0000000000000001/"7"/true/越界值；拒绝重复键（含转义后的同名键）、尾随逗号、非法 UTF-8/Unicode 转义和第17层容器，接受第16层。使用包含转义引号和字符串内括号的样例，防止错误深度检查。
12. 所有者主动离房、断线和发送关闭请求均只产生一次服务器关闭中止；没有重复等待状态或释放后读取。固定端口占用、启动失败及客户端强制退出按 BUILD.md 验证。

## 15. 未来扩展边界

- AI 通过个人可见视图产生相同规则操作，服务端内部调用，不需要虚构 TCP 连接。
- 重连需要稳定身份凭据、会话保留和恢复快照，不能只凭昵称实现。
- 多房间对外开放需新增列表或选择接口，当前 join_default 不变。
- 不兼容字段/语义修改升级 version；兼容新增可选字段保持 v1。
- 公网使用需另行设计加密与认证，此规范以受信任局域网为范围。
