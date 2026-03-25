cmake_minimum_required(VERSION 3.16)
project(MyDiscordBot)

# Use modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find DPP (installed system-wide earlier)
find_package(dpp REQUIRED)

# Add your executable
add_executable(bot
    src/main.cpp
    # add more .cpp files here if needed
)

# Link DPP
target_link_libraries(bot PRIVATE dpp::dpp)

# Optional: warnings (recommended)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bot PRIVATE -Wall -Wextra -Wpedantic)
endif()
