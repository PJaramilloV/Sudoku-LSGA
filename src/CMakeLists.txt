include(CheckLanguage)
find_package(OpenCL)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
  message("${PROJECT_NAME}: CUDA FOUND")
  add_subdirectory(cuda)
endif()
if(OpenCL_FOUND)
  message("${PROJECT_NAME}: OpenCL FOUND")
  add_subdirectory(cl)
endif()


set(CPU_SOURCES main.cpp member.cpp)
add_executable(${PROJECT_NAME}CPU ${CPU_SOURCES})

