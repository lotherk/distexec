cmake_minimum_required(VERSION 3.0)

project (distexec LANGUAGES C VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Debug)

set(DISTEXEC_HOMEPAGE "https://github.com/lotherk/distexec")
set(DISTEXEC_WIKIPAGE "https://github.com/lotherk/distexec/wiki")

include(FindPkgConfig)
find_package(PkgConfig)
#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPack.txt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src) # for gengetopt...

set(LOGGER_FORMAT_DEFAULT "%D %T.%X [%N] %L %f:%m:%l: %M")
set(LOGGER_FORMAT_DATE    "%Y-%m-%d")
set(LOGGER_FORMAT_TIME    "%H:%M:%S")

set(PLUGIN_PATH_DEFAULT ${CMAKE_INSTALL_PREFIX}/share/distexec/plugins)

option(LOGGER_DEBUG "Enable logger debug messages" ON)
option(LOGGER_ENABLE "Enable logger messages" ON)

option(FILE_PLUGIN "Enable file backend" OFF)
option(FOREMAN_PLUGIN "Enable foreman backend" OFF)
option(SSH_PLUGIN "Enable ssh frontend" OFF)
option(DUMMY_PLUGIN "Enable dummy plugin" OFF)
option(EXEC_PLUGIN "Enable exec plugin" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

CONFIGURE_FILE(cmdline.ggo.in cmdline.ggo @ONLY)
CONFIGURE_FILE(config.h.in ../include/config.h @ONLY)

execute_process(COMMAND gengetopt -C --input cmdline.ggo --output-dir ../src --unamed-opts=SCRIPTS)
execute_process(COMMAND git log OUTPUT_FILE ChangeLog.txt)
execute_process(COMMAND doxygen WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_definitions("-Wfatal-errors")

add_library(libdistexec SHARED 
		src/logger.c
		src/distexec.c
		src/plugin.c
)

set_target_properties(libdistexec PROPERTIES PREFIX "")

pkg_check_modules(LIBPCRE REQUIRED libpcre)
include_directories(${LIBPCRE_INCLUDE_DIR}) # does not work on osxcross
target_link_libraries(libdistexec ${LIBPCRE_LDFLAGS})
target_link_libraries(libdistexec "dl")
target_link_libraries(libdistexec "pthread")
target_link_libraries(libdistexec "m")



add_executable(distexec src/main.c src/cmdline.c)
target_link_libraries(distexec libdistexec)

#add_executable(test-uri tests/test-uri.c)
#target_link_libraries(test-uri libdistexec)

#add_executable(test-popen3 tests/test-popen3.c)
#target_link_libraries(test-popen3 libdistexec)

#install(TARGETS distexec libdistexec
#	RUNTIME DESTINATION bin
#	LIBRARY DESTINATION lib
#	ARCHIVE DESTINATION lib/static)

# backends


#install(DIRECTORY include/distexec DESTINATION include/)
#install(DIRECTORY examples DESTINATION share/distexec)
#install(DIRECTORY doc DESTINATION share/distexec)

#install(FILES
#	COPYING	
	#	build/ChangeLog.txt
    #	DESTINATION share/distexec)


    #message(STATUS "")

#include(CTest

