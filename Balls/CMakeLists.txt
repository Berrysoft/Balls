include(../XamlCpp/cmake/Modules/XamlCompilerHelper.cmake)
xaml_fix_rtti_flags()
xaml_fix_warning_flags()
xaml_fix_char8_t_flags()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB BALLS_SOURCE "src/*.cpp")
add_executable(balls ${BALLS_SOURCE})
if(WIN32)
	set_target_properties(balls PROPERTIES WIN32_EXECUTABLE ON)
    target_sources(balls PUBLIC res/logo.rc)
elseif(APPLE)
    set_target_properties(balls PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_ICON_FILE logo
    )
    target_sources(balls PUBLIC res/logo.icns)
    set_source_files_properties(res/logo.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()
target_include_directories(balls PUBLIC include)
target_link_libraries(balls PUBLIC xaml_ui_canvas xaml_ui_appmain nowide PRIVATE stream_format)

if(APPLE)
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/balls.app \"\" \"${XAML_LIBRARY_OUTPUT_DIRECTORY}\")
        execute_process(COMMAND
            ${CMAKE_COMMAND} -E tar \"cfv\" \"${CMAKE_INSTALL_PREFIX}/balls.app.zip\" --format=zip
                \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/balls.app\"
            WORKING_DIRECTORY \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\"
        )
    ")
else()
    install(TARGETS balls xaml_global xaml_meta xaml_markup xaml_ui xaml_ui_canvas nowide
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()
