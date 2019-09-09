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

#include "QueryD3D12Impl.h"
#include "EngineMemory.h"
#include "RenderDeviceD3D12Impl.h"
namespace Diligent
{
    
QueryD3D12Impl :: QueryD3D12Impl(IReferenceCounters*    pRefCounters,
                                 RenderDeviceD3D12Impl* pDevice,
                                 const QueryDesc&       Desc) : 
    TQueryBase(pRefCounters, pDevice, Desc)
{
}

QueryD3D12Impl :: ~QueryD3D12Impl()
{
}

D3D12_QUERY_TYPE QueryD3D12Impl :: GetD3DType() const
{
	switch (m_Desc.Type)
	{
	case QUERY_TYPE_BINARY_OCCLUSION: return D3D12_QUERY_TYPE_BINARY_OCCLUSION;
	case QUERY_TYPE_OCCLUSION: return D3D12_QUERY_TYPE_OCCLUSION;
	case QUERY_TYPE_TIMESTAMP: return D3D12_QUERY_TYPE_TIMESTAMP;
	}
	return D3D12_QUERY_TYPE_OCCLUSION;
}



}
