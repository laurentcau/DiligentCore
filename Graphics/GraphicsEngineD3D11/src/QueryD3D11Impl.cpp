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

#include "pch.h"
#include <atlbase.h>

#include "QueryD3D11Impl.h"
#include "EngineMemory.h"

namespace Diligent
{
    
QueryD3D11Impl :: QueryD3D11Impl(IReferenceCounters*    pRefCounters,
                                 RenderDeviceD3D11Impl* pDevice,
                                 const QueryDesc&       Desc) : 
    TQueryBase(pRefCounters, pDevice, Desc)
{
	auto* pd3d11Device = ValidatedCast<RenderDeviceD3D11Impl>(pDevice)->GetD3D11Device();
	D3D11_QUERY_DESC dx11desc;
	dx11desc.Query = D3D11_QUERY_OCCLUSION;
	switch (Desc.Type)
	{
	case QUERY_TYPE_BINARY_OCCLUSION:
	case QUERY_TYPE_OCCLUSION: dx11desc.Query = D3D11_QUERY_OCCLUSION; break;
	case QUERY_TYPE_TIMESTAMP: dx11desc.Query = D3D11_QUERY_TIMESTAMP; break;
	default:
		break;
	}
	dx11desc.MiscFlags = 0;
	pd3d11Device->CreateQuery(&dx11desc, &m_pDx11Query);
}

QueryD3D11Impl :: ~QueryD3D11Impl()
{
}

QUERY_STATUS QueryD3D11Impl::GetValue(Uint64& v)
{
	if (!m_pd3d11Ctx)
		return QUERY_STATUS_RETRY;

	HRESULT res;
	CComPtr<ID3D11DeviceContext> pCtx;
	res = m_pd3d11Ctx->GetData(m_pDx11Query, &v, sizeof(Uint64), 0);

	if (S_FALSE == res)
	{
		v = 0;
		return QUERY_STATUS_RETRY;
	}
	return QUERY_STATUS_SUCESS;
}

}
