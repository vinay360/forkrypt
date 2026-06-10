cmake_minimum_required(VERSION 3.16)

project(forkrypt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(forkrypt_lib
    src/bitstream.cpp
    src/file_io.cpp
    src/game_state.cpp
    src/pgn_writer.cpp
    src/pgn_reader.cpp
    src/encoder.cpp
    src/decoder.cpp
)

target_include_directories(forkrypt_lib PUBLIC include)

target_compile_options(forkrypt_lib PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_executable(forkrypt src/main.cpp)
target_link_libraries(forkrypt PRIVATE forkrypt_lib)

add_executable(forkrypt_server src/server_main.cpp)
target_link_libraries(forkrypt_server PRIVATE forkrypt_lib pthread)

enable_testing()

add_executable(roundtrip_tests tests/roundtrip_tests.cpp)
target_link_libraries(roundtrip_tests PRIVATE forkrypt_lib)
add_test(NAME roundtrip_tests COMMAND roundtrip_tests)
