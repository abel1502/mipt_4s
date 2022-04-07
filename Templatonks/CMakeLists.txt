cmake_minimum_required(VERSION 3.8)

project("Templatonks")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS true)


set(CXX_WARNING_FLAGS "-W4 -wd4201 -wd4146 -wd4100 -wd4189 -wd26812 -wd33011")
set(CXX_OPTIMIZE_FLAGS "-O2")

set(CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_OPTIMIZE_FLAGS} -EHsc")


add_library(ACL STATIC IMPORTED)
set(ACL_DIR "${PROJECT_SOURCE_DIR}/../../ACL/")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(ACL_BUILD_TYPE "x64/Debug")
else ()
	set(ACL_BUILD_TYPE "x64/Release")
endif ()

set_target_properties(ACL PROPERTIES
	IMPORTED_LOCATION "${ACL_DIR}/lib/${ACL_BUILD_TYPE}/ACL.lib"
	INTERFACE_INCLUDE_DIRECTORIES "${ACL_DIR}/include/"
)


add_executable(
	templatonks

	"main.cpp"
	"array.h"
	"storage.h"
	"dot.h"
	"test.h"
	"allocator.h"
	"test.cpp"
 )

target_link_libraries(
	templatonks

	ACL
)
