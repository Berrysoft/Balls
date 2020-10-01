project(BallsExecutable VERSION 2.2.4 LANGUAGES CXX)

file(GLOB BALLS_SOURCE "src/*.cpp")
add_executable(balls ${BALLS_SOURCE})

set(BALLS_VERSION ${PROJECT_VERSION})
set(BALLS_COPYRIGHT "Copyright (c) 2018-2020 Berrysoft")
set(BALLS_DESCRIPTION "二维弹球")

find_package(Git)
if(Git_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_HASH
    )
    if(GIT_RESULT EQUAL 0)
        string(REPLACE "\n" "" GIT_HASH ${GIT_HASH})
        string(SUBSTRING ${GIT_HASH} 0 6 GIT_HASH)
        string(CONCAT BALLS_LONG_VERSION ${BALLS_VERSION} "-" ${GIT_HASH})
    endif()
else()
    set(BALLS_LONG_VERSION ${BALLS_VERSION})
endif()

target_compile_definitions(balls PRIVATE "BALLS_VERSION=\"${BALLS_LONG_VERSION}\"")

if(WIN32)
    set_target_properties(balls PROPERTIES WIN32_EXECUTABLE ON)
    get_filename_component(BALLS_ICON "res/logo.ico" ABSOLUTE)
    set(BALLS_WIN32_VERSION "${PROJECT_VERSION_MAJOR}, ${PROJECT_VERSION_MINOR}, ${PROJECT_VERSION_PATCH}")
    configure_file(res/info.rc.in ${PROJECT_BINARY_DIR}/info.rc @ONLY)
    target_sources(balls PUBLIC ${PROJECT_BINARY_DIR}/info.rc)
elseif(APPLE)
    set_target_properties(balls PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_ICON_FILE logo
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${BALLS_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${BALLS_LONG_VERSION}
        MACOSX_BUNDLE_COPYRIGHT ${BALLS_COPYRIGHT}
        MACOSX_BUNDLE_INFO_STRING ${BALLS_DESCRIPTION}
    )
    target_sources(balls PUBLIC res/logo.icns)
    set_source_files_properties(res/logo.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()
if(MSVC)
    target_compile_options(balls PUBLIC "/utf-8")
endif()
target_include_directories(balls PUBLIC include)
target_link_libraries(balls PUBLIC balls_lib xaml_ui_canvas xaml_ui_appmain nowide PRIVATE stream_format)

if(APPLE)
    if(${BUILD_SHARED_LIBS})
        install(CODE "
            include(BundleUtilities)
            fixup_bundle(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/balls.app \"\" \"${XAML_LIBRARY_OUTPUT_DIRECTORY}\")
        ")
    endif()
    install(CODE "
        execute_process(COMMAND
            ${CMAKE_COMMAND} -E tar \"cfv\" \"${CMAKE_INSTALL_PREFIX}/balls.app.zip\" --format=zip
                \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/balls.app\"
            WORKING_DIRECTORY \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\"
        )
    ")
else()
    set(__INSTALL_TARGETS balls)
    if(${BUILD_SHARED_LIBS})
        list(APPEND __INSTALL_TARGETS xaml_global xaml_meta xaml_markup xaml_ui xaml_ui_canvas nowide balls_lib)
    endif()
    install(TARGETS ${__INSTALL_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()
