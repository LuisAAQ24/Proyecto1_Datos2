# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\gui-profiler_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\gui-profiler_autogen.dir\\ParseCache.txt"
  "gui-profiler_autogen"
  )
endif()
