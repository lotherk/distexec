cmake_minimum_required(VERSION 3.0)

project (distexec LANGUAGES C VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Debug)

set(DISTEXEC_HOMEPAGE "https://github.com/lotherk/distexec")
set(DISTEXEC_WIKIPAGE "https://github.com/lotherk/distexec/wiki")

include(FindPkgConfig)
find_package(PkgConfig)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPack.txt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src) # for gengetopt...

set(LOGGER_FORMAT_DEFAULT "%D %T.%X [%N] %L %f:%m:%l: %M")
set(LOGGER_FORMAT_DATE    "%Y-%m-%d")
set(LOGGER_FORMAT_TIME    "%H:%M:%S")

set(PLUGIN_PATH_DEFAULT ${CMAKE_INSTALL_PREFIX}/share/distexec/plugins)

option(LOGGER_DEBUG "Enable logger debug messages" ON)
option(LOGGER_ENABLE "Enable logger messages" ON)

option(FILE_PLUGIN "Enable file backend" ON)
option(FOREMAN_PLUGIN "Enable foreman backend" ON)
option(SSH_PLUGIN "Enable ssh frontend" ON)
option(DUMMY_PLUGIN "Enable dummy plugin" ON)
option(EXEC_PLUGIN "Enable exec plugin" ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

CONFIGURE_FILE(cmdline.ggo.in cmdline.ggo @ONLY)
CONFIGURE_FILE(config.h.in ../include/config.h @ONLY)

#execute_process(COMMAND gengetopt -C --input cmdline.ggo --output-dir ../src --unamed-opts=SCRIPTS)
execute_process(COMMAND git log OUTPUT_FILE ChangeLog.txt)
execute_process(COMMAND doxygen WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_definitions("-Wfatal-errors")

add_library(libdistexec SHARED 
		src/util.c
		src/logger.c
		src/error.c
		src/thread.c
		src/node.c
		src/plugin.c
		src/uri.c
		src/popen3.c
		src/distexec.c
)

if(FOREMAN_PLUGIN)
	add_library(foreman SHARED src/plugins/foreman.c)
	target_compile_definitions(foreman PUBLIC DISTEXEC_DLL_H)
	SET_TARGET_PROPERTIES(foreman PROPERTIES PREFIX "")
	target_link_libraries(foreman libdistexec)
	if(WIN32)
		target_link_libraries(foreman ${CMAKE_SOURCE_DIR}/ext/windows/lib/libjson-c.dll)
		target_link_libraries(foreman ${CMAKE_SOURCE_DIR}/ext/windows/lib/libcurl.dll)
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext/windows/include)
		install(TARGETS foreman RUNTIME DESTINATION share/distexec/plugins)
	else(WIN32)
		pkg_check_modules(LIBJSONC REQUIRED json-c)
		include_directories(${LIBJSONC_INCLUDE_DIR})
		target_link_libraries(foreman ${LIBJSONC_LDFLAGS})

		pkg_check_modules(LIBCURL REQUIRED libcurl)
		include_directories(${LIBCURL_INCLUDE_DIR})
		target_link_libraries(foreman ${LIBCURL_LDFLAGS})
		install(TARGETS foreman LIBRARY DESTINATION share/distexec/plugins)
	endif(WIN32)
endif(FOREMAN_PLUGIN)

if(FILE_PLUGIN)
	add_library(file SHARED src/plugins/file.c)
	target_compile_definitions(file PUBLIC DISTEXEC_DLL_H)
	SET_TARGET_PROPERTIES(file PROPERTIES PREFIX "")
	target_link_libraries(file libdistexec)
	if(WIN32)
		install(TARGETS file RUNTIME DESTINATION share/distexec/plugins)
	else(WIN32)
		install(TARGETS file LIBRARY DESTINATION share/distexec/plugins)
	endif(WIN32)
endif(FILE_PLUGIN)

if(SSH_PLUGIN)
	add_library(ssh SHARED src/plugins/ssh.c)
	target_compile_definitions(ssh PUBLIC DISTEXEC_DLL_H)
	SET_TARGET_PROPERTIES(ssh PROPERTIES PREFIX "")
	target_link_libraries(ssh libdistexec)
	if(WIN32)
		target_link_libraries(ssh ${CMAKE_SOURCE_DIR}/ext/windows/lib/libssh2-1.dll)
		target_link_libraries(ssh ws2_32)
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext/windows/include)
		install(TARGETS ssh RUNTIME DESTINATION share/distexec/plugins)
	else(WIN32)
		pkg_check_modules(LIBSSH2 REQUIRED libssh2)
		include_directories(${LIBSSH2_INCLUDE_DIR})
		target_link_libraries(ssh ${LIBSSH2_LDFLAGS})
		install(TARGETS ssh LIBRARY DESTINATION share/distexec/plugins)
	endif(WIN32)
endif(SSH_PLUGIN)

if(EXEC_PLUGIN)
	add_library(exec SHARED src/plugins/exec.c)
	target_compile_definitions(exec PUBLIC DISTEXEC_DLL_H)
	SET_TARGET_PROPERTIES(exec PROPERTIES PREFIX "")
	target_link_libraries(exec libdistexec)
	if(WIN32)
		install(TARGETS exec RUNTIME DESTINATION share/distexec/plugins)
	else(WIN32)
		install(TARGETS exec LIBRARY DESTINATION share/distexec/plugins)
	endif(WIN32)
endif(EXEC_PLUGIN)
if(DUMMY_PLUGIN)
	add_library(dummy SHARED src/plugins/dummy.c)
	target_compile_definitions(dummy PUBLIC DISTEXEC_DLL_H)
	SET_TARGET_PROPERTIES(dummy PROPERTIES PREFIX "")
	target_link_libraries(dummy libdistexec)
	if(WIN32)
		install(TARGETS dummy RUNTIME DESTINATION share/distexec/plugins)
	else(WIN32)
		install(TARGETS dummy LIBRARY DESTINATION share/distexec/plugins)
	endif(WIN32)
endif(DUMMY_PLUGIN)

set_target_properties(libdistexec PROPERTIES OUTPUT_NAME "distexec")
target_compile_definitions(libdistexec PUBLIC DISTEXEC_DLL_H)

if(WIN32)
	set(DLL ON)
	target_link_libraries(libdistexec "winmm")
	target_link_libraries(libdistexec "shlwapi")
	target_link_libraries(libdistexec ${CMAKE_SOURCE_DIR}/ext/windows/lib/libpcre32-0.dll)
else(WIN32)
	pkg_check_modules(LIBPCRE REQUIRED libpcre)
	include_directories(${LIBPCRE_INCLUDE_DIR}) # does not work on osxcross
	target_link_libraries(libdistexec ${LIBPCRE_LDFLAGS})
	target_link_libraries(libdistexec "dl")
	target_link_libraries(libdistexec "pthread")
	target_link_libraries(libdistexec "m")
endif(WIN32)



add_executable(distexec src/main.c src/copa.c)
target_link_libraries(distexec libdistexec)

add_executable(test-uri tests/test-uri.c)
target_link_libraries(test-uri libdistexec)

add_executable(test-popen3 tests/test-popen3.c)
target_link_libraries(test-popen3 libdistexec)

install(TARGETS distexec libdistexec
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)

# backends


install(DIRECTORY include/distexec DESTINATION include/)
install(DIRECTORY examples DESTINATION share/distexec)
install(DIRECTORY doc DESTINATION share/distexec)

install(FILES
	COPYING	
	#	build/ChangeLog.txt
	DESTINATION share/distexec)


message(STATUS "")

#include(CTest

