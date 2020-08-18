include(../XamlCpp/cmake/Modules/XamlCompilerHelper.cmake)
xaml_fix_rtti_flags()
xaml_fix_warning_flags()
xaml_fix_char8_t_flags()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(balls_lib)
add_subdirectory(balls)
