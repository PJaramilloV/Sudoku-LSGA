include(CheckLanguage)
find_package(OpenCL)
if(OpenCL_FOUND)
  message("${PROJECT_NAME}: OpenCL FOUND")
  add_subdirectory(cl)
endif()


set(CPU_SOURCES main.cpp member.cpp)
add_executable(${PROJECT_NAME}CPU ${CPU_SOURCES})

