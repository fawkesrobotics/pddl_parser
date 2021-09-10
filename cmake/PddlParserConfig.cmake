include(CMakeFindDependencyMacro)
find_dependency(Boost COMPONENTS system)
find_dependency(spdlog)
include("${CMAKE_CURRENT_LIST_DIR}/PddlParserTargets.cmake")
