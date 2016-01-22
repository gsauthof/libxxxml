cmake_minimum_required(VERSION 2.8)
#cmake_minimum_required(VERSION 3.0.2)

project(libxxxml CXX)

IF(${CMAKE_PROJECT_NAME} STREQUAL "libxxxml")
  include (${CMAKE_SOURCE_DIR}/cmake/modules/coverage.cmake)
  include (${CMAKE_SOURCE_DIR}/cmake/modules/sanitize.cmake)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall")

  # for unittests
  find_package(Boost 1.55
    COMPONENTS
      unit_test_framework
      regex
    REQUIRED
    )
endif() # CMAKE_PROJECT_NAME

find_library(XML2_LIB NAMES xml2 HINTS /opt/csw/lib/64)
find_path(XML2_INCLUDE_DIR libxml/xmlreader.h PATH_SUFFIXES libxml2
  HINTS /opt/csw/include)

set(LIB_SRC
  xxxml/xxxml.cc
  xxxml/util.cc
  )

add_library(xxxml SHARED
  ${LIB_SRC}
  )
add_library(xxxml_static STATIC
  ${LIB_SRC}
  )

set_property(TARGET xxxml xxxml_static PROPERTY INCLUDE_DIRECTORIES
  # needed for <regex>, i.e. boost/regex.hpp if gcc <= 4.9
  # cf. xxxml/util.cc
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${XML2_INCLUDE_DIR}
  )

# under windows shared/static libraries have the same extension ...
if(${UNIX})
  set_target_properties(xxxml_static PROPERTIES OUTPUT_NAME xxxml)
endif()

# guard from super-projects, i.e. when it is added as subdirectory
IF(${CMAKE_PROJECT_NAME} STREQUAL "libxxxml")
  add_executable(ut
    test/main.cc
    test/xxxml.cc
    test/util.cc
    )
  set_property(TARGET ut PROPERTY INCLUDE_DIRECTORIES
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${XML2_INCLUDE_DIR}
    )
  target_link_libraries(ut
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    xxxml_static
    ${XML2_LIB}
    )
  # for executing it from a quickfix environment
  add_custom_target(check COMMAND ut)

endif() # CMAKE_PROJECT_NAME
