# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/NaTruKi_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/NaTruKi_autogen.dir/ParseCache.txt"
  "NaTruKi_autogen"
  "src/svg/CMakeFiles/svg_lib_autogen.dir/AutogenUsed.txt"
  "src/svg/CMakeFiles/svg_lib_autogen.dir/ParseCache.txt"
  "src/svg/svg_lib_autogen"
  "src/ui/CMakeFiles/ui_lib_autogen.dir/AutogenUsed.txt"
  "src/ui/CMakeFiles/ui_lib_autogen.dir/ParseCache.txt"
  "src/ui/ui_lib_autogen"
  )
endif()
