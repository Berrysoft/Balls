project(BallsLib CXX)

file(GLOB BALLS_LIB_SOURCE "src/*.cpp")
add_library(balls_lib ${BALLS_LIB_SOURCE})
target_include_directories(balls_lib
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(balls_lib PUBLIC xaml_ui)

if(${BUILD_SHARED_LIBS})
    target_compile_definitions(balls_lib PRIVATE "BALLS_LIB_API=__XAML_EXPORT")
endif()
