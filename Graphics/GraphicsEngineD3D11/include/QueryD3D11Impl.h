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
/// Declaration of Diligent::QueryD3D11Impl class

#include <deque>
#include "QueryD3D11.h"
#include "RenderDeviceD3D11.h"
#include "QueryBase.h"
#include "RenderDeviceD3D11Impl.h"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Implementation of the Diligent::IQueryD3D11 interface
class QueryD3D11Impl final : public QueryBase<IQueryD3D11, RenderDeviceD3D11Impl>
{
public:
    using TQueryBase = QueryBase<IQueryD3D11, RenderDeviceD3D11Impl>;

    QueryD3D11Impl(IReferenceCounters*    pRefCounters,
                   RenderDeviceD3D11Impl* pDevice,
                   const QueryDesc&       Desc);
    ~QueryD3D11Impl();

	CComPtr<ID3D11Query> &GetD3D11Query() { return m_pDx11Query; }

	QUERY_STATUS GetValue(Uint64& v);

	void SetDeviceContext(CComPtr<ID3D11DeviceContext> context) { m_pd3d11Ctx = context; }

private:
	CComPtr<ID3D11Query> m_pDx11Query;
	CComPtr<ID3D11DeviceContext> m_pd3d11Ctx;
};

}
