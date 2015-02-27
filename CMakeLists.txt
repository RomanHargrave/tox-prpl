cmake_minimum_required(VERSION 2.8)
project(tox-prpl)

add_definitions(
	-DVERSION="0.4.2"
	-DPACKAGE_URL="http://tox.dhs.org/"
	)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(GLIB REQUIRED)
include_directories(${GLIB_INCLUDE_DIRS})

find_package(Libtox REQUIRED core)
include_directories(${LIBTOX_INCLUDE_DIRS})

find_package(LibPurple 2.7.0 REQUIRED)
include_directories(${LIBPURPLE_INCLUDE_DIRS})

set(LIBS ${LIBS} 
	${GLIB_LIBRARIES} 
	${LIBTOX_LIBRARIES} 
	${LIBPURPLE_LIBRARIES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(SOURCE_FILES
	src/util.c
	src/purple/account.c
	src/purple/buddy.c
	src/purple/chat.c
	src/purple/commands.c
	src/purple/xfers.c
	src/tox/buddy.c
	src/tox/chat.c
	src/tox/xfers.c
	src/common/xfers.c
	src/toxprpl.c)

include_directories(include)

add_library(toxprpl ${SOURCE_FILES})

target_link_libraries(toxprpl ${LIBS})
