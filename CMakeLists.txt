# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.14)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project("libHidTest")

set(EXEC_NAME "hidpp_mon")

add_subdirectory("${CMAKE_SOURCE_DIR}/external/hidapi-win")
# add_subdirectory("${CMAKE_SOURCE_DIR}/external/websocketpp")
# add_subdirectory("${CMAKE_SOURCE_DIR}/external/nlohmann_json")


add_executable(${EXEC_NAME} 
	"src/CMakeProject1.cpp" 
	"src/HIDDevice.cpp" 
	"src/HIDPPMsg.cpp"
 "src/LogiDevice.cpp" "src/LGSTrayHID_common.cpp" "src/Battery1001.cpp" "src/Battery1000.cpp" "src/Battery1004.cpp" "src/MapWrapper.cpp" "src/hid_io_debug.cpp")

target_link_libraries(
	${EXEC_NAME} PUBLIC
	hidapi_win
)
target_include_directories(
	${EXEC_NAME} PUBLIC 
	"include"
)

if(MSVC)
	target_compile_options(${EXEC_NAME} 
		PRIVATE "/external:anglebrackets"
		PRIVATE "/external:W0"
	)
endif(MSVC)