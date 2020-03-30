/*     Copyright 2019 Diligent Graphics LLC
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#pragma once

/// \file
/// Implementation of the Diligent::ShaderBase template class

#include <vector>

#include "Shader.h"
#include "DeviceObjectBase.h"
#include "STDAllocator.h"
#include "PlatformMisc.h"
#include "EngineMemory.h"
#include "Align.h"

namespace Diligent
{

inline SHADER_TYPE GetShaderTypeFromIndex(Int32 Index)
{
    return static_cast<SHADER_TYPE>(1 << Index);
}

inline Int32 GetShaderTypeIndex(SHADER_TYPE Type)
{
    VERIFY(IsPowerOfTwo(Uint32{Type}), "Only single shader stage should be provided");

    Int32 ShaderIndex = PlatformMisc::GetLSB(Type);

#ifdef DE_DEBUG
    switch( Type )
    {
        case SHADER_TYPE_UNKNOWN: VERIFY_EXPR(ShaderIndex == -1); break;
        case SHADER_TYPE_VERTEX:  VERIFY_EXPR(ShaderIndex ==  0); break;
        case SHADER_TYPE_PIXEL:   VERIFY_EXPR(ShaderIndex ==  1); break;
        case SHADER_TYPE_GEOMETRY:VERIFY_EXPR(ShaderIndex ==  2); break;
        case SHADER_TYPE_HULL:    VERIFY_EXPR(ShaderIndex ==  3); break;
        case SHADER_TYPE_DOMAIN:  VERIFY_EXPR(ShaderIndex ==  4); break;
        case SHADER_TYPE_COMPUTE: VERIFY_EXPR(ShaderIndex ==  5); break;
        default: UNEXPECTED( "Unexpected shader type (", Type, ")" ); break;
    }
    VERIFY( Type == GetShaderTypeFromIndex(ShaderIndex), "Incorrect shader type index" );
#endif
    return ShaderIndex;
}

static const int VSInd = GetShaderTypeIndex(SHADER_TYPE_VERTEX);
static const int PSInd = GetShaderTypeIndex(SHADER_TYPE_PIXEL);
static const int GSInd = GetShaderTypeIndex(SHADER_TYPE_GEOMETRY);
static const int HSInd = GetShaderTypeIndex(SHADER_TYPE_HULL);
static const int DSInd = GetShaderTypeIndex(SHADER_TYPE_DOMAIN);
static const int CSInd = GetShaderTypeIndex(SHADER_TYPE_COMPUTE);


/// Template class implementing base functionality for a shader object

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IShaderD3D11, Diligent::IShaderD3D12,
///                          Diligent::IShaderGL or Diligent::IShaderVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
///                                (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                                 Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
template<class BaseInterface, class RenderDeviceImplType>
class ShaderBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, ShaderDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, ShaderDesc>;

    /// \param pRefCounters - reference counters object that controls the lifetime of this shader.
	/// \param pDevice - pointer to the device.
	/// \param ShdrDesc - shader description.
	/// \param bIsDeviceInternal - flag indicating if the shader is an internal device object and 
	///							   must not keep a strong reference to the device.
    ShaderBase(IReferenceCounters* pRefCounters, RenderDeviceImplType* pDevice, const ShaderDesc& ShdrDesc, bool bIsDeviceInternal = false) :
        TDeviceObjectBase(pRefCounters, pDevice, ShdrDesc, bIsDeviceInternal)
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE( IID_Shader, TDeviceObjectBase )
};

}
