/*     Copyright 2019 Laurent Caumont
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
/// Implementation of the Diligent::QueryBase template class

#include "DeviceObjectBase.h"
#include "GraphicsTypes.h"
#include "RefCntAutoPtr.h"

namespace Diligent
{

class IRenderDevice;
class IQuery;

/// Template class implementing base functionality for a Query object

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IQueryD3D11, Diligent::IQueryD3D12,
///                          Diligent::IQueryGL or Diligent::IQueryVk).
/// \tparam RenderDeviceImplType - type of the render device implementation
template<class BaseInterface, class RenderDeviceImplType>
class QueryBase : public DeviceObjectBase<BaseInterface, RenderDeviceImplType, QueryDesc>
{
public:
    typedef DeviceObjectBase<BaseInterface, RenderDeviceImplType, QueryDesc> TDeviceObjectBase;

    /// \param pRefCounters      - reference counters object that controls the lifetime of this command list.
    /// \param Desc              - Query description
	/// \param pDevice           - pointer to the device.
	/// \param bIsDeviceInternal - flag indicating if the Query is an internal device object and 
	///							   must not keep a strong reference to the device.
    QueryBase( IReferenceCounters* pRefCounters, RenderDeviceImplType* pDevice, const QueryDesc& Desc, bool bIsDeviceInternal = false ) :
        TDeviceObjectBase( pRefCounters, pDevice, Desc, bIsDeviceInternal )
    {}

    ~QueryBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE( IID_Query, TDeviceObjectBase )
};

}
