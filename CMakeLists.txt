CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
PROJECT(Seeklet)

SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
SET(CMAKE_AUTOMOC ON)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
	ADD_COMPILE_OPTIONS(-O3)
ELSE()
	ADD_COMPILE_OPTIONS(-O0)
ENDIF()

FIND_PACKAGE(OpenSSL REQUIRED)
MESSAGE("OpenSSL version: ${OPENSSL_VERSION}")
LINK_LIBRARIES("${OPENSSL_LIBRARIES}")

FIND_PACKAGE(Qt6 COMPONENTS Core Network Sql WebEngineCore WebEngineWidgets REQUIRED)
LINK_LIBRARIES(Qt6::Core Qt6::Network Qt6::Sql Qt6::WebEngineCore Qt6::WebEngineWidgets)

LINK_LIBRARIES(jansson htmlcxx)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME}
	main.hpp
	main.cpp
	crawler.hpp
	crawler.cpp
	configuration_keeper.hpp
	configuration_keeper.cpp
	indexer.hpp
	indexer.cpp
	simple_hash_func.hpp
	simple_hash_func.cpp
	web_page_processor.hpp
	web_page_processor.cpp
	util.hpp
	util.cpp)
