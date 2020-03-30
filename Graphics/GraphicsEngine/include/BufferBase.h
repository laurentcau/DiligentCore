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
/// Implementation of the Diligent::BufferBase template class

#include "Buffer.h"
#include "GraphicsTypes.h"
#include "DeviceObjectBase.h"
#include "GraphicsAccessories.h"
#include "STDAllocator.h"
#include <memory>

namespace Diligent
{

/// Template class implementing base functionality for a buffer object

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IBufferD3D11, Diligent::IBufferD3D12,
///                          Diligent::IBufferGL or Diligent::IBufferVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
///                                (Diligent::RenderDeviceD3D11Impl, Diligent::RenderDeviceD3D12Impl,
///                                 Diligent::RenderDeviceGLImpl, or Diligent::RenderDeviceVkImpl)
/// \tparam BufferViewImplType - type of the buffer view implementation
///                              (Diligent::BufferViewD3D11Impl, Diligent::BufferViewD3D12Impl,
///                               Diligent::BufferViewGLImpl or Diligent::BufferViewVkImpl)
/// \tparam TBuffViewObjAllocator - type of the allocator that is used to allocate memory for the buffer view object instances
template<class BaseInterface, class RenderDeviceImplType, class BufferViewImplType, class TBuffViewObjAllocator>
class BufferBase : public DeviceObjectBase <BaseInterface, RenderDeviceImplType, BufferDesc>
{
public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, BufferDesc>;

    /// \param pRefCounters - reference counters object that controls the lifetime of this buffer.
    /// \param BuffViewObjAllocator - allocator that is used to allocate memory for the buffer view instances.
    ///                               This parameter is only used for debug purposes.
	/// \param pDevice - pointer to the device.
	/// \param BuffDesc - buffer description.
	/// \param bIsDeviceInternal - flag indicating if the buffer is an internal device object and 
	///							   must not keep a strong reference to the device.
    BufferBase( IReferenceCounters*    pRefCounters,
                TBuffViewObjAllocator& BuffViewObjAllocator, 
                RenderDeviceImplType*  pDevice, 
                const BufferDesc&      BuffDesc,
                bool                   bIsDeviceInternal) :
        TDeviceObjectBase( pRefCounters, pDevice, BuffDesc, bIsDeviceInternal),
#ifdef DE_DEBUG
        m_dbgBuffViewAllocator(BuffViewObjAllocator),
#endif
        m_pDefaultUAV(nullptr, STDDeleter<BufferViewImplType, TBuffViewObjAllocator>(BuffViewObjAllocator) ),
        m_pDefaultSRV(nullptr, STDDeleter<BufferViewImplType, TBuffViewObjAllocator>(BuffViewObjAllocator) )
    {
#   define VERIFY_BUFFER(Expr, ...) if (!(Expr)) LOG_ERROR_AND_THROW("Buffer '",  this->m_Desc.Name ? this->m_Desc.Name : "", "': ", ##__VA_ARGS__)

        Uint32 AllowedBindFlags =
            BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_UNIFORM_BUFFER |
            BIND_SHADER_RESOURCE | BIND_STREAM_OUTPUT | BIND_UNORDERED_ACCESS |
            BIND_INDIRECT_DRAW_ARGS;
        const Char* strAllowedBindFlags =
            "BIND_VERTEX_BUFFER (1), BIND_INDEX_BUFFER (2), BIND_UNIFORM_BUFFER (4), "
            "BIND_SHADER_RESOURCE (8), BIND_STREAM_OUTPUT (16), BIND_UNORDERED_ACCESS (128), "
            "BIND_INDIRECT_DRAW_ARGS (256)";

        VERIFY_BUFFER( (BuffDesc.BindFlags & ~AllowedBindFlags) == 0, "Incorrect bind flags specified (", BuffDesc.BindFlags & ~AllowedBindFlags, "). Only the following flags are allowed:\n", strAllowedBindFlags );
        
        if( (this->m_Desc.BindFlags & BIND_UNORDERED_ACCESS) ||
            (this->m_Desc.BindFlags & BIND_SHADER_RESOURCE) )
        {
            VERIFY_BUFFER( this->m_Desc.Mode > BUFFER_MODE_UNDEFINED && this->m_Desc.Mode < BUFFER_MODE_NUM_MODES, GetBufferModeString(this->m_Desc.Mode), " is not a valid mode for a buffer created with BIND_SHADER_RESOURCE or BIND_UNORDERED_ACCESS flags" );
            if (this->m_Desc.Mode == BUFFER_MODE_STRUCTURED || this->m_Desc.Mode == BUFFER_MODE_FORMATTED)
            {
                VERIFY_BUFFER( this->m_Desc.ElementByteStride != 0, "Element stride must not be zero for structured and formatted buffers" );
            }
            else if (this->m_Desc.Mode == BUFFER_MODE_RAW)
            {

            }
        }

#   undef VERIFY_BUFFER

        Uint64 DeviceQueuesMask = pDevice->GetCommandQueueMask();
        DEV_CHECK_ERR( (this->m_Desc.CommandQueueMask & DeviceQueuesMask) != 0, "No bits in the command queue mask (0x", std::hex, this->m_Desc.CommandQueueMask, ") correspond to one of ", pDevice->GetCommandQueueCount(), " available device command queues");
        this->m_Desc.CommandQueueMask &= DeviceQueuesMask;
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE( IID_Buffer, TDeviceObjectBase )

    /// Implementation of IBuffer::CreateView(); calls CreateViewInternal() virtual function
    /// that creates buffer view for the specific engine implementation.
    virtual void CreateView( const struct BufferViewDesc& ViewDesc, IBufferView** ppView )override;

    /// Implementation of IBuffer::GetDefaultView().
    virtual IBufferView* GetDefaultView( BUFFER_VIEW_TYPE ViewType )override;

    /// Creates default buffer views.

    /// 
    /// - Creates default shader resource view addressing the entire buffer if Diligent::BIND_SHADER_RESOURCE flag is set
    /// - Creates default unordered access view addressing the entire buffer if Diligent::BIND_UNORDERED_ACCESS flag is set 
    ///
    /// The function calls CreateViewInternal().
    void CreateDefaultViews();

    virtual void SetState(RESOURCE_STATE State)override final
    {
        this->m_State = State;
    }

    virtual RESOURCE_STATE GetState() const override final
    {
        return this->m_State;
    }

    bool IsInKnownState() const 
    {
        return this->m_State != RESOURCE_STATE_UNKNOWN;
    }

    bool CheckState(RESOURCE_STATE State)const
    {
        VERIFY((State & (State-1)) == 0, "Single state is expected");
        VERIFY(IsInKnownState(), "Buffer state is unknown");
        return (this->m_State & State) == State;
    }

protected:

    /// Pure virtual function that creates buffer view for the specific engine implementation.
    virtual void CreateViewInternal( const struct BufferViewDesc& ViewDesc, IBufferView **ppView, bool bIsDefaultView ) = 0;

    /// Corrects buffer view description and validates view parameters.
    void CorrectBufferViewDesc( struct BufferViewDesc& ViewDesc );

#ifdef DE_DEBUG
    TBuffViewObjAllocator& m_dbgBuffViewAllocator;
#endif

    RESOURCE_STATE m_State = RESOURCE_STATE_UNKNOWN;

    /// Default UAV addressing the entire buffer
    std::unique_ptr<BufferViewImplType, STDDeleter<BufferViewImplType, TBuffViewObjAllocator> > m_pDefaultUAV;

    /// Default SRV addressing the entire buffer
    std::unique_ptr<BufferViewImplType, STDDeleter<BufferViewImplType, TBuffViewObjAllocator> > m_pDefaultSRV;
};




template<class BaseInterface, class RenderDeviceImplType, class BufferViewImplType, class TBuffViewObjAllocator>
void BufferBase<BaseInterface, RenderDeviceImplType, BufferViewImplType, TBuffViewObjAllocator> :: CreateView( const struct BufferViewDesc &ViewDesc, IBufferView **ppView )
{
    DEV_CHECK_ERR(ViewDesc.ViewType != BUFFER_VIEW_UNDEFINED, "Buffer view type is not specified");
    if (ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE)
        DEV_CHECK_ERR(this->m_Desc.BindFlags & BIND_SHADER_RESOURCE, "Attempting to create SRV for buffer '", this->m_Desc.Name, "' that was not created with BIND_SHADER_RESOURCE flag");
    else if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS)
        DEV_CHECK_ERR(this->m_Desc.BindFlags & BIND_UNORDERED_ACCESS, "Attempting to create UAV for buffer '", this->m_Desc.Name, "' that was not created with BIND_UNORDERED_ACCESS flag");
    else
        UNEXPECTED("Unexpected buffer view type");

    CreateViewInternal( ViewDesc, ppView, false );
}


template<class BaseInterface, class RenderDeviceImplType, class BufferViewImplType, class TBuffViewObjAllocator>
void BufferBase<BaseInterface, RenderDeviceImplType, BufferViewImplType, TBuffViewObjAllocator> :: CorrectBufferViewDesc(struct BufferViewDesc& ViewDesc)
{
    if( ViewDesc.ByteWidth == 0 )
    {
        DEV_CHECK_ERR(this->m_Desc.uiSizeInBytes > ViewDesc.ByteOffset, "Byte offset (", ViewDesc.ByteOffset, ") exceeds buffer size (", this->m_Desc.uiSizeInBytes, ")");
        ViewDesc.ByteWidth = this->m_Desc.uiSizeInBytes - ViewDesc.ByteOffset;
    }
    if( ViewDesc.ByteOffset + ViewDesc.ByteWidth > this->m_Desc.uiSizeInBytes )
        LOG_ERROR_AND_THROW( "Buffer view range [", ViewDesc.ByteOffset, ", ", ViewDesc.ByteOffset + ViewDesc.ByteWidth, ") is out of the buffer boundaries [0, ", this->m_Desc.uiSizeInBytes, ")." );
    if( (this->m_Desc.BindFlags & BIND_UNORDERED_ACCESS) ||
        (this->m_Desc.BindFlags & BIND_SHADER_RESOURCE) )
    {
        if (this->m_Desc.Mode == BUFFER_MODE_STRUCTURED || this->m_Desc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY( this->m_Desc.ElementByteStride != 0, "Element byte stride is zero" );
            if( (ViewDesc.ByteOffset % this->m_Desc.ElementByteStride) != 0 )
                LOG_ERROR_AND_THROW( "Buffer view byte offset (", ViewDesc.ByteOffset, ") is not multiple of element byte stride (", this->m_Desc.ElementByteStride, ")." );
            if( (ViewDesc.ByteWidth % this->m_Desc.ElementByteStride) != 0 )
                LOG_ERROR_AND_THROW( "Buffer view byte width (", ViewDesc.ByteWidth, ") is not multiple of element byte stride (", this->m_Desc.ElementByteStride, ")." );
        }

        if (this->m_Desc.Mode == BUFFER_MODE_FORMATTED && ViewDesc.Format.ValueType == VT_UNDEFINED)
            LOG_ERROR_AND_THROW("Format must be specified when creating a view of a formatted buffer");

        if (this->m_Desc.Mode == BUFFER_MODE_FORMATTED || (this->m_Desc.Mode == BUFFER_MODE_RAW && ViewDesc.Format.ValueType != VT_UNDEFINED))
        {
            if (ViewDesc.Format.NumComponents <= 0 || ViewDesc.Format.NumComponents > 4)
                LOG_ERROR_AND_THROW("Incorrect number of components (", Uint32{ViewDesc.Format.NumComponents}, "). 1, 2, 3, or 4 are allowed values");
            if (ViewDesc.Format.ValueType == VT_FLOAT32 || ViewDesc.Format.ValueType == VT_FLOAT16)
                ViewDesc.Format.IsNormalized = false;
            auto ViewElementStride = GetValueSize( ViewDesc.Format.ValueType ) * Uint32{ViewDesc.Format.NumComponents};
            if (this->m_Desc.Mode == BUFFER_MODE_RAW && this->m_Desc.ElementByteStride == 0)
                LOG_ERROR_AND_THROW("To enable formatted views of a raw buffer, element byte must be specified during buffer initialization");
            if (ViewElementStride != this->m_Desc.ElementByteStride)
                LOG_ERROR_AND_THROW("Buffer element byte stride (", this->m_Desc.ElementByteStride, ") is not consistent with the size (", ViewElementStride, ") defined by the format of the view (", GetBufferFormatString(ViewDesc.Format), ')' );
        }

        if (this->m_Desc.Mode == BUFFER_MODE_RAW && ViewDesc.Format.ValueType == VT_UNDEFINED)
        {
            if ((ViewDesc.ByteOffset % 16) != 0)
                LOG_ERROR_AND_THROW("When creating a RAW view, the offset of the first element from the start of the buffer (", ViewDesc.ByteOffset, ") must be a multiple of 16 bytes");
        }
    }
}

template<class BaseInterface, class RenderDeviceImplType, class BufferViewImplType, class TBuffViewObjAllocator>
IBufferView* BufferBase<BaseInterface, RenderDeviceImplType, BufferViewImplType, TBuffViewObjAllocator> ::GetDefaultView( BUFFER_VIEW_TYPE ViewType )
{
    switch( ViewType )
    {
        case BUFFER_VIEW_SHADER_RESOURCE:  return m_pDefaultSRV.get();
        case BUFFER_VIEW_UNORDERED_ACCESS: return m_pDefaultUAV.get();
        default: UNEXPECTED( "Unknown view type" ); return nullptr;
    }
}

template<class BaseInterface, class RenderDeviceImplType, class BufferViewImplType, class TBuffViewObjAllocator>
void BufferBase<BaseInterface, RenderDeviceImplType, BufferViewImplType, TBuffViewObjAllocator> :: CreateDefaultViews()
{
    // Create default views for structured and raw buffers. For formatted buffers we do not know the view format, so
    // cannot create views.

    if (this->m_Desc.BindFlags & BIND_UNORDERED_ACCESS && (this->m_Desc.Mode == BUFFER_MODE_STRUCTURED || this->m_Desc.Mode == BUFFER_MODE_RAW))
    {
        BufferViewDesc ViewDesc;
        ViewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
        IBufferView* pUAV = nullptr;
        CreateViewInternal( ViewDesc, &pUAV, true );
        m_pDefaultUAV.reset( static_cast<BufferViewImplType*>(pUAV) );
        VERIFY( m_pDefaultUAV->GetDesc().ViewType == BUFFER_VIEW_UNORDERED_ACCESS, "Unexpected view type" );
    }

    if (this->m_Desc.BindFlags & BIND_SHADER_RESOURCE && (this->m_Desc.Mode == BUFFER_MODE_STRUCTURED || this->m_Desc.Mode == BUFFER_MODE_RAW))
    {
        BufferViewDesc ViewDesc;
        ViewDesc.ViewType = BUFFER_VIEW_SHADER_RESOURCE;
        IBufferView* pSRV = nullptr;
        CreateViewInternal( ViewDesc, &pSRV, true );
        m_pDefaultSRV.reset( static_cast<BufferViewImplType*>(pSRV) );
        VERIFY( m_pDefaultSRV->GetDesc().ViewType == BUFFER_VIEW_SHADER_RESOURCE, "Unexpected view type"  );
    }
}

}
