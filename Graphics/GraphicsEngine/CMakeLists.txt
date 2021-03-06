cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsEngine CXX)

set(INCLUDE 
    include/BufferBase.h
    include/BufferViewBase.h
    include/CommandListBase.h
    include/DefaultShaderSourceStreamFactory.h
    include/Defines.h
    include/DeviceContextBase.h
    include/DeviceObjectBase.h
    include/EngineFactoryBase.h
    include/EngineMemory.h
    include/FenceBase.h
    include/pch.h
    include/PipelineStateBase.h
    include/RenderDeviceBase.h
    include/ResourceMappingImpl.h
    include/SamplerBase.h
    include/ShaderBase.h
    include/ShaderResourceBindingBase.h
    include/ShaderResourceVariableBase.h
    include/StateObjectsRegistry.h
    include/SwapChainBase.h
    include/TextureBase.h
    include/TextureViewBase.h
)

set(INTERFACE 
    interface/APIInfo.h
    interface/BlendState.h
    interface/Buffer.h
    interface/BufferView.h
    interface/CommandList.h
    interface/Constants.h
    interface/DepthStencilState.h
    interface/DeviceCaps.h
    interface/DeviceContext.h
    interface/DeviceObject.h
    interface/EngineFactory.h
    interface/Fence.h
    interface/GraphicsTypes.h
    interface/InputLayout.h
    interface/MapHelper.h
    interface/PipelineState.h
    interface/RasterizerState.h
    interface/RenderDevice.h
    interface/ResourceMapping.h
    interface/Sampler.h
    interface/Shader.h
    interface/ShaderResourceBinding.h
    interface/ShaderResourceVariable.h
    interface/SwapChain.h
    interface/Texture.h
    interface/TextureView.h
)

set(SOURCE
    src/APIInfo.cpp
    src/DefaultShaderSourceStreamFactory.cpp
    src/EngineMemory.cpp
    src/ResourceMapping.cpp
    src/Texture.cpp
)

add_library(Diligent-GraphicsEngine STATIC ${SOURCE} ${INTERFACE} ${INCLUDE})
set_common_target_properties(Diligent-GraphicsEngine)

target_include_directories(Diligent-GraphicsEngine 
PUBLIC
    include
    interface
)

target_link_libraries(Diligent-GraphicsEngine 
PUBLIC
    Diligent-BuildSettings
    Diligent-PlatformInterface
    Diligent-Common
    Diligent-GraphicsAccessories
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngine PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(INSTALL_DILIGENT_CORE)
    install_core_lib(Diligent-GraphicsEngine)
endif()