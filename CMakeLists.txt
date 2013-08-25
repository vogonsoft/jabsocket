cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
project(jabsocket)

set(CMAKE_C_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(jabsocket base64.c cmanager.c framer.c log.c main.c parseconfig.c
	rqparser.c streamparse.c util.c wsserver.c wsmessage.c)

set (jabsocket_VERSION_MAJOR 0)
set (jabsocket_VERSION_MINOR 1)
set (jabsocket_VERSION_PATCH 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/jabsocketConfig.h.in"
  "${PROJECT_BINARY_DIR}/jabsocketConfig.h"
  )

find_package(Expat REQUIRED)
if (EXPAT_FOUND)
include_directories(${EXPAT_INCLUDE_DIR})
set(LIBS ${LIBS} ${EXPAT_LIBRARY})
endif (EXPAT_FOUND)

find_package(YAML REQUIRED)
if (YAML_FOUND)
include_directories(${YAML_INCLUDE_DIR})
set(LIBS ${LIBS} ${YAML_LIBRARY})
endif (YAML_FOUND)

find_package(Event REQUIRED)
if (EVENT_FOUND)
include_directories(${EVENT_INCLUDE_DIR})
set(LIBS ${LIBS} ${EVENT_LIBRARY})
endif (EVENT_FOUND)

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
include_directories(${OPENSSL_INCLUDE_DIR})
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

target_link_libraries(jabsocket ${LIBS})

enable_testing()

add_executable(jabsocket_test
	AllTests.c
	base64_test.c config_test.c framer_test.c
	streamparse_test.c util_test.c rqparser_test.c CuTest.c
	base64.c parseconfig.c framer.c streamparse.c util.c
	wsmessage.c wsmessage_test.c
	rqparser.c log.c)

target_link_libraries(jabsocket_test ${LIBS})

add_custom_target(check COMMAND ctest -V)

target_link_libraries(check ${LIBS})
add_test(check jabsocket_test)
add_dependencies(check jabsocket_test)

# add the install targets
install (TARGETS jabsocket DESTINATION bin)
install (FILES "${PROJECT_BINARY_DIR}/jabsocket.conf"        
         DESTINATION etc)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION_MAJOR "${jabsocket_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${jabsocket_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${jabsocket_VERSION_PATCH}")

SET(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
include (CPack)

