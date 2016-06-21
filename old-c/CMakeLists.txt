CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)
PROJECT(v9ttd C)

include(appversion.cmake)

include_directories(${CMAKE_SOURCE_DIR}/include)

ADD_DEFINITIONS(-Wall -std=c99 -DV9TTD_VERSION="${APP_VERSION}")

if(DEBUG)
	add_definitions(-DDEBUG -g -O0)
else(DEBUG)
	add_definitions(-DNDEBUG -O3)
endif(DEBUG)


ADD_SUBDIRECTORY(src)

# vim: tabstop=4
