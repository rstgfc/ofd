# CMakeLists.txt - OFD Viewer Plugin for Directory Opus 13
cmake_minimum_required(VERSION 3.15)

project(OFDViewerPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32 -D_WINDOWS -D_USRDLL -D_WINDLL)
endif()

# Create DLL
add_library(${PROJECT_NAME} SHARED
    OFDViewerPlugin.cpp
    OFDViewerPlugin.h
    OFDViewerPlugin.def
)

# Set output name
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "OFDViewerPlugin"
    PREFIX ""
    SUFFIX ".dll"
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        gdiplus
        oleaut32
        shlwapi
        uuid
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /WX-
        /EHsc
    )
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Register DLL after build (requires admin privileges)
# Uncomment the following lines if you want automatic registration
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND regsvr32 /s \"$<TARGET_FILE:${PROJECT_NAME}>\"
#     COMMENT "Registering OFD Viewer Plugin..."
# )
