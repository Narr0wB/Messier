
project(
    'messier', 
    'cpp', 
    default_options: [
        'cpp_std=c++20',
        'b_lto=true',
        'buildtype=release',
    ], 
    version: '1.0.0', license: 'MIT'
)

add_project_arguments('-U_LIBCPP_ENABLE_ASSERTIONS', language: 'cpp')
add_project_arguments('-march=native', language: 'cpp')

inc = include_directories('include')

# Dive into the src folder, and build the executable
subdir('src')