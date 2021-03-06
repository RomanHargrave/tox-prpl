cmake_minimum_required(VERSION 2.8)
project(tox-prpl)
set(CMAKE_BUILD_TYPE Release)

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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")

set(SOURCE_FILES

	# Misc.
	src/util.c

	# LibPurple Specific
	src/purple/account.c
	src/purple/commands.c

	# Chat Backend
	src/tox/chat.c
	src/purple/chat.c

	# Buddy Backend
	src/tox/buddy.c
	src/purple/buddy.c

	# Group Chat Backend
	src/common/group_chat.c
	src/tox/group_chat.c

	# Transfers Implementation
	src/common/xfers.c
	src/tox/xfers.c
	src/purple/xfers.c

	src/toxprpl.c)

include_directories(include)

add_library(toxprpl SHARED ${SOURCE_FILES})

target_link_libraries(toxprpl ${LIBS})
