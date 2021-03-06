cmake_minimum_required (VERSION 3.6)

project(Diligent-Primitives CXX)

set(SOURCE
    src/DebugOutput.cpp
    src/test.cpp
)

set(INTERFACE
    interface/BasicTypes.h
    interface/DataBlob.h
    interface/DebugOutput.h
    interface/FlagEnum.h
    interface/Errors.h
    interface/FileStream.h
    interface/FormatString.h
    interface/InterfaceID.h
    interface/MemoryAllocator.h
    interface/Object.h
    interface/ReferenceCounters.h
)

# This should be an interface library. However, CMake does not show
# interface libraries in an IDE, so we have to create a static library instead
add_library(Diligent-Primitives STATIC ${INTERFACE} ${SOURCE})
set_common_target_properties(Diligent-Primitives)

target_link_libraries(Diligent-Primitives 
PUBLIC
    Diligent-BuildSettings
)

target_include_directories(Diligent-Primitives
PUBLIC
    interface
)

source_group("interface" FILES ${INTERFACE})
source_group("src" FILES ${SOURCE})

set_target_properties(Diligent-Primitives PROPERTIES
    FOLDER DiligentCore
)

if(INSTALL_DILIGENT_CORE)
    install_core_lib(Diligent-Primitives)
endif()