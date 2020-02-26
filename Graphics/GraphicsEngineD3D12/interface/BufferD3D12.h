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
/// Definition of the Diligent::IBufferD3D12 interface

#include "../../GraphicsEngine/interface/Buffer.h"

namespace Diligent
{

class IDeviceContext;

// {3E9B15ED-A289-48DC-8214-C6E3E6177378}
static constexpr INTERFACE_ID IID_BufferD3D12 =
{ 0x3e9b15ed, 0xa289, 0x48dc, { 0x82, 0x14, 0xc6, 0xe3, 0xe6, 0x17, 0x73, 0x78 } };

/// Interface to the buffer object implemented in D3D12
class IBufferD3D12 : public IBuffer
{
public:

    /// Returns a pointer to the ID3D12Resource interface of the internal Direct3D12 object.

    /// The method does *NOT* call AddRef() on the returned interface,
    /// so Release() must not be called.
    /// \param [in] DataStartByteOffset - Offset from the beginning of the buffer
    ///                            to the start of the data. This parameter
    ///                            is required for dynamic buffers, which are
    ///                            suballocated in a dynamic upload heap
    /// \param [in] pContext - Device context within which address of the buffer is requested.
    virtual ID3D12Resource* GetD3D12Buffer(size_t& DataStartByteOffset, IDeviceContext* pContext) = 0;

    /// Sets the buffer usage state

    /// \param [in] state - D3D12 resource state to be set for this buffer
    virtual void SetD3D12ResourceState(D3D12_RESOURCE_STATES state) = 0;

    /// Returns current D3D12 buffer state. 
    /// If the state is unknown to the engine (Diligent::RESOURCE_STATE_UNKNOWN), 
    /// returns D3D12_RESOURCE_STATE_COMMON (0).
    virtual D3D12_RESOURCE_STATES GetD3D12ResourceState()const = 0;

    //to get dynamic buffer GPU address
    virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(Uint32 ContextId, IDeviceContext* pContext) = 0;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCBVHandle() const = 0;
};

}
