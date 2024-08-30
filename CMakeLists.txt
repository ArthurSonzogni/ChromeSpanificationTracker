cmake_minimum_required (VERSION 3.15)

include(FetchContent)

FetchContent_Declare(subprocess
  GIT_REPOSITORY https://github.com/benman64/subprocess
  GIT_TAG e1cae5e86e5d089e65e906f7c13917b7bbb75d04
  GIT_PROGRESS TRUE
  GIT_SHALLOW FALSE
)

FetchContent_GetProperties(subprocess)
if(NOT subprocess_POPULATED)
  FetchContent_Populate(subprocess)
  add_subdirectory(
    ${subprocess_SOURCE_DIR}
    ${subprocess_BINARY_DIR}
    EXCLUDE_FROM_ALL
  )
endif()

# ------------------------------------------------------------------------------
project(chrome-spanification-tracker LANGUAGES CXX)

add_executable(chrome-spanification-tracker
  src/main.cpp
  src/parser.cpp
  src/parser.hpp
)

target_include_directories(chrome-spanification-tracker
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(chrome-spanification-tracker
  PRIVATE subprocess
)

target_compile_options(chrome-spanification-tracker
  PRIVATE "-Wall"
  PRIVATE "-Wextra"
  PRIVATE "-pedantic"
  PRIVATE "-Werror"
  PRIVATE "-Wdeprecated"
)

set_target_properties(chrome-spanification-tracker
  PROPERTIES CXX_STANDARD 20)

install(TARGETS chrome-spanification-tracker
  RUNTIME DESTINATION "bin")
