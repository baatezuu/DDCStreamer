cmake_minimum_required(VERSION 3.21)
project(DDCStreamer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# User configurable options
option(ENABLE_LOGGING "Enable spdlog logging" ON)
option(SIMULATION_ONLY "Build without requiring DDC SDK libraries (simulation mode)" OFF)

# DDC SDK root (adjust as needed)
set(DDC_SDK_ROOT "C:/DDC/aceXtremeSDKv4.9.5" CACHE PATH "Path to DDC aceXtreme SDK root")

if(NOT SIMULATION_ONLY)
    if(NOT EXISTS "${DDC_SDK_ROOT}/include")
        message(FATAL_ERROR "DDC SDK include directory not found at ${DDC_SDK_ROOT}/include. Set DDC_SDK_ROOT or enable SIMULATION_ONLY.")
    endif()
endif()

add_library(ddc_streamer
    src/B1553Monitor.cpp
    src/MessageParser.cpp
    src/UdpPublisher.cpp
    src/JsonFormatter.cpp
    src/Config.cpp
    src/ExtractionEngine.cpp
    src/CsvLogger.cpp
)

# Include dirs
target_include_directories(ddc_streamer
    PUBLIC
        include
        $<IF:$<BOOL:${SIMULATION_ONLY}>,,${DDC_SDK_ROOT}/include>
)

# nlohmann_json (try find, fallback to FetchContent if available)
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    set(FETCHCONTENT_QUIET ON)
    FetchContent_Declare(nlohmann_json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
    FetchContent_MakeAvailable(nlohmann_json)
endif()
target_link_libraries(ddc_streamer PUBLIC nlohmann_json::nlohmann_json)

# Platform / SDK libs (placeholder - adjust to actual DDC .lib names)
if (WIN32 AND NOT SIMULATION_ONLY)
    find_library(ACE_API_LIB NAMES aceXtremeAPI PATHS ${DDC_SDK_ROOT}/lib)
    if(ACE_API_LIB)
        find_library(ACE_SDK_LIB NAMES aceXtreme PATHS ${DDC_SDK_ROOT}/lib)
        target_link_libraries(ddc_streamer PRIVATE ${ACE_API_LIB} ${ACE_SDK_LIB} ws2_32)
    else()
        message(WARNING "aceXtreme libraries not found; enable SIMULATION_ONLY to skip linking.")
    endif()
elseif (WIN32)
    target_link_libraries(ddc_streamer PRIVATE ws2_32)
endif()

# Logging library (header only if spdlog installed via package manager or vendored)
if(ENABLE_LOGGING)
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        target_link_libraries(ddc_streamer PRIVATE spdlog::spdlog_header_only)
        target_compile_definitions(ddc_streamer PRIVATE DDC_ENABLE_LOGGING=1)
    else()
        target_compile_definitions(ddc_streamer PRIVATE DDC_ENABLE_LOGGING=0)
    endif()
endif()

add_executable(DDCStreamerApp src/Runner.cpp)

target_link_libraries(DDCStreamerApp PRIVATE ddc_streamer)

if(SIMULATION_ONLY)
    target_compile_definitions(ddc_streamer PRIVATE DDC_SIMULATION_ONLY=1)
endif()

# Install rules
install(TARGETS DDCStreamerApp RUNTIME DESTINATION bin)
install(TARGETS ddc_streamer ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
