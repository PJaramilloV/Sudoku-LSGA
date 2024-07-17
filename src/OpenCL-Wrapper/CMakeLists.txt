find_package(OpenCL REQUIRED)
add_executable(${PROJECT_NAME}CL  src/main.cpp src/kernel.cpp)
target_link_libraries(${PROJECT_NAME}CL PRIVATE OpenCL::OpenCL)