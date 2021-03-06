cmake_minimum_required (VERSION 3.8)

project(Diligent-UniversalWindowsPlatform CXX)

set(INTERFACE 
    interface/UWPDebug.h
    interface/UWPFileSystem.h
    interface/UWPDefinitions.h
    ../Win32/interface/Win32Atomics.h
)

set(SOURCE 
    src/UWPDebug.cpp
    src/UWPFileSystem.cpp
    ../Win32/src/Win32Atomics.cpp
)

add_library(Diligent-UniversalWindowsPlatform ${SOURCE} ${INTERFACE} ${PLATFORM_INTERFACE_HEADERS})
set_common_target_properties(Diligent-UniversalWindowsPlatform)

target_include_directories(Diligent-UniversalWindowsPlatform 
PUBLIC
    ../Win32/interface
    interface
)

target_link_libraries(Diligent-UniversalWindowsPlatform 
PUBLIC
    Diligent-BuildSettings 
    Diligent-BasicPlatform
    Diligent-PlatformInterface
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${PLATFORM_INTERFACE_HEADERS})

set_target_properties(Diligent-UniversalWindowsPlatform PROPERTIES
    FOLDER DiligentCore/Platforms
)

if(INSTALL_DILIGENT_CORE)
    install_core_lib(Diligent-UniversalWindowsPlatform)
endif()
