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
/// Declaration of Diligent::QueryD3D12Impl class

#include "QueryD3D12.h"
#include "RenderDeviceD3D12.h"
#include "QueryBase.h"
#include "RenderDeviceD3D12Impl.h"

namespace Diligent
{

/// Implementation of the Diligent::IQueryD3D12 interface
class QueryD3D12Impl final : public QueryBase<IQueryD3D12, RenderDeviceD3D12Impl>
{
public:
    using TQueryBase = QueryBase<IQueryD3D12, RenderDeviceD3D12Impl>;

    QueryD3D12Impl(IReferenceCounters*     pRefCounters,
                   RenderDeviceD3D12Impl*  pDevice,
                   const QueryDesc&        Desc);
    ~QueryD3D12Impl();

	void SetSlot(Uint64 s) { m_Slot = s; }
	Uint64 GetSlot() const { return m_Slot; }

	virtual QUERY_STATUS GetValue(Uint64& v) const override final
	{ v = m_Value; return m_Status; }

	void SetValue(Uint64& v) 
	{ m_Status = QUERY_STATUS_SUCESS; m_Value = v; }

	D3D12_QUERY_TYPE GetD3DType() const;
	void Reset()
	{
		m_Value = 0;
		m_Status = QUERY_STATUS_RETRY;
	}
private:
	Uint64 m_Slot;
	Uint64 m_Value = 0;
	QUERY_STATUS m_Status = QUERY_STATUS_RETRY;
};

}
