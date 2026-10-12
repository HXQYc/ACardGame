
# =========================
# 通用卡牌模块
# =========================

add_library(acard_cards STATIC
    cards/card.h
    cards/deck.h
    cards/deck.cpp
)

# 对外提供头文件搜索路径。
# 使用方可以通过 #include "cards/deck.h" 引用接口。
target_include_directories(acard_cards PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# =========================
# 捉黑 A 游戏规则模块
# =========================

add_library(acard_game STATIC
    game/game_deck.h
    game/game_deck.cpp
)

target_include_directories(acard_game PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 公共头文件使用 Card，因此依赖设为 PUBLIC。
target_link_libraries(acard_game PUBLIC
    acard_cards
)

# =========================
# 服务端可执行程序
# =========================

add_executable(acard_server
    app/main.cpp
)

# 构建目标名是 acard_server，输出程序名是 ACardServer。
# Windows 下会自动添加 .exe 后缀。
set_target_properties(acard_server PROPERTIES
    OUTPUT_NAME ACardServer
)

# 服务端链接卡牌模块，同时获得其 PUBLIC 头文件搜索路径。
target_link_libraries(acard_server PRIVATE
    acard_game
)

# =========================
# 编译器配置
# =========================

# 仅 MSVC 使用此选项，保证源码与执行字符集为 UTF-8。
if(MSVC)
    target_compile_options(acard_cards PRIVATE /utf-8)
    target_compile_options(acard_game PRIVATE /utf-8)
    target_compile_options(acard_server PRIVATE /utf-8)
endif()