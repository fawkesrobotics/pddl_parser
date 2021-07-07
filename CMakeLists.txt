cmake_minimum_required(VERSION 3.12)

project(pddl_parser VERSION 0.9.0
	DESCRIPTION "Basic PDDL parser based on boost spirit")

find_package(Boost REQUIRED COMPONENTS system)

add_library(pddl_parser SHARED pddl_parser.cpp pddl_semantics.cpp)
target_link_libraries(pddl_parser PUBLIC Boost::boost Boost::system)
target_include_directories(
  pddl_parser PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(pddl_parser PROPERTIES SOVERSION
                                               ${PROJECT_VERSION_MAJOR})

include(GNUInstallDirs)

install(
  TARGETS pddl_parser
	EXPORT PddlParserTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/pddl_parser DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/PddlParserConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  EXPORT PddlParserTargets
  FILE PddlParserTargets.cmake
  NAMESPACE PddlParser::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PddlParser)

install(FILES cmake/PddlParserConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/PddlParserConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PddlParser)

get_target_property(PKG_CONFIG_PUBLIC_LIBS pddl_parser INTERFACE_LINK_LIBRARIES)
get_target_property(PKG_CONFIG_PRIVATE_LIBS pddl_parser LINK_LIBRARIES)
get_target_property(PKG_CONFIG_CFLAGS pddl_parser COMPILE_FLAGS)
if ("${PKG_CONFIG_CFLAGS}" STREQUAL "PKG_CONFIG_CFLAGS-NOTFOUND")
  set(PKG_CONFIG_CFLAGS "")
endif()

set(PKG_CONFIG_PUBLIC_LIBS " -lboost_system")
#set(PKG_CONFIG_PUBLIC_LIBS "-lboost_thread -lboost_system")
set(PKG_CONFIG_PRIVATE_LIBS "")
set(PKG_CONFIG_CFLAGS "")

configure_file(pddl_parser.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pddl_parser.pc @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pddl_parser.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
