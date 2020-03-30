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

#include "pch.h"
#include "ShaderResourceBindingVkImpl.h"
#include "PipelineStateVkImpl.h"
#include "ShaderVkImpl.h"
#include "RenderDeviceVkImpl.h"

namespace Diligent
{

ShaderResourceBindingVkImpl::ShaderResourceBindingVkImpl(IReferenceCounters* pRefCounters, PipelineStateVkImpl* pPSO, bool IsPSOInternal) :
    TBase
    {
        pRefCounters,
        pPSO,
        IsPSOInternal
    },
    m_ShaderResourceCache{ShaderResourceCacheVk::DbgCacheContentType::SRBResources}
{
    auto* ppShaders = pPSO->GetShaders();
    m_NumShaders = static_cast<decltype(m_NumShaders)>(pPSO->GetNumShaders());

    auto* pRenderDeviceVkImpl = pPSO->GetDevice();
    // This will only allocate memory and initialize descriptor sets in the resource cache
    // Resources will be initialized by InitializeResourceMemoryInCache()
    auto& ResourceCacheDataAllocator = pPSO->GetSRBMemoryAllocator().GetResourceCacheDataAllocator(0);
    pPSO->GetPipelineLayout().InitResourceCache(pRenderDeviceVkImpl, m_ShaderResourceCache, ResourceCacheDataAllocator, pPSO->GetDesc().Name);
    
    m_pShaderVarMgrs = ALLOCATE(GetRawAllocator(), "Raw memory for ShaderVariableManagerVk", ShaderVariableManagerVk, m_NumShaders);

    for (Uint32 s = 0; s < m_NumShaders; ++s)
    {
        auto* pShader = ppShaders[s];
        auto ShaderType = pShader->GetDesc().ShaderType;
        auto ShaderInd = GetShaderTypeIndex(ShaderType);
        m_ResourceLayoutIndex[ShaderInd] = static_cast<Int8>(s);

        auto& VarDataAllocator = pPSO->GetSRBMemoryAllocator().GetShaderVariableDataAllocator(s);

        const auto& SrcLayout = pPSO->GetShaderResLayout(s);
        // Use source layout to initialize resource memory in the cache
        SrcLayout.InitializeResourceMemoryInCache(m_ShaderResourceCache);

        // Create shader variable manager in place
        // Initialize vars manager to reference mutable and dynamic variables
        // Note that the cache has space for all variable types
        const SHADER_RESOURCE_VARIABLE_TYPE VarTypes[] = {SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
        new (m_pShaderVarMgrs + s) ShaderVariableManagerVk(*this, SrcLayout, VarDataAllocator, VarTypes, _countof(VarTypes), m_ShaderResourceCache);
    }
#ifdef DE_DEBUG
    m_ShaderResourceCache.DbgVerifyResourceInitialization();
#endif

}

ShaderResourceBindingVkImpl::~ShaderResourceBindingVkImpl()
{
    PipelineStateVkImpl* pPSO = ValidatedCast<PipelineStateVkImpl>(m_pPSO);
    for(Uint32 s = 0; s < m_NumShaders; ++s)
    {
        auto& VarDataAllocator = pPSO->GetSRBMemoryAllocator().GetShaderVariableDataAllocator(s);
        m_pShaderVarMgrs[s].DestroyVariables(VarDataAllocator);
        m_pShaderVarMgrs[s].~ShaderVariableManagerVk();
    }

    GetRawAllocator().Free(m_pShaderVarMgrs);
}

IMPLEMENT_QUERY_INTERFACE( ShaderResourceBindingVkImpl, IID_ShaderResourceBindingVk, TBase )

void ShaderResourceBindingVkImpl::BindResources(Uint32 ShaderFlags, IResourceMapping *pResMapping, Uint32 Flags)
{
    for (auto ShaderInd = 0; ShaderInd <= CSInd; ++ShaderInd )
    {
        if (ShaderFlags & GetShaderTypeFromIndex(ShaderInd))
        {
            auto ResLayoutInd = m_ResourceLayoutIndex[ShaderInd];
            if(ResLayoutInd >= 0)
            {
                m_pShaderVarMgrs[ResLayoutInd].BindResources(pResMapping, Flags);
            }
        }
    }
}

IShaderResourceVariable* ShaderResourceBindingVkImpl::GetVariableByName(SHADER_TYPE ShaderType, const char* Name)
{
    auto ShaderInd = GetShaderTypeIndex(ShaderType);
    auto ResLayoutInd = m_ResourceLayoutIndex[ShaderInd];
    if (ResLayoutInd < 0)
    {
        LOG_WARNING_MESSAGE("Unable to find mutable/dynamic variable '", Name, "': shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " is inactive in Pipeline State '", m_pPSO->GetDesc().Name, "'.");
        return nullptr;
    }
    return m_pShaderVarMgrs[ResLayoutInd].GetVariable(Name);
}

Uint32 ShaderResourceBindingVkImpl::GetVariableCount(SHADER_TYPE ShaderType) const
{
    auto ShaderInd = GetShaderTypeIndex(ShaderType);
    auto ResLayoutInd = m_ResourceLayoutIndex[ShaderInd];
    if (ResLayoutInd < 0)
    {
        LOG_WARNING_MESSAGE("Unable to get the number of mutable/dynamic variables: shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " is inactive in Pipeline State '", m_pPSO->GetDesc().Name, "'.");
        return 0;
    }
    return m_pShaderVarMgrs[ResLayoutInd].GetVariableCount();
}

IShaderResourceVariable* ShaderResourceBindingVkImpl::GetVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    auto ShaderInd = GetShaderTypeIndex(ShaderType);
    auto ResLayoutInd = m_ResourceLayoutIndex[ShaderInd];
    if (ResLayoutInd < 0)
    {
        LOG_WARNING_MESSAGE("Unable to get mutable/dynamic variable at index ", Index, ": shader stage ", GetShaderTypeLiteralName(ShaderType),
                            " is inactive in Pipeline State '", m_pPSO->GetDesc().Name, "'.");
        return nullptr;
    }
    return m_pShaderVarMgrs[ResLayoutInd].GetVariable(Index);
}

void ShaderResourceBindingVkImpl::InitializeStaticResources(const IPipelineState* pPipelineState)
{
    if (StaticResourcesInitialized())
    {
        LOG_WARNING_MESSAGE("Static resources have already been initialized in this shader resource binding object. The operation will be ignored.");
        return;
    }

    if (pPipelineState == nullptr)
    {
        pPipelineState = GetPipelineState();
    }
    else
    {
        DEV_CHECK_ERR(pPipelineState->IsCompatibleWith(GetPipelineState()), "The pipeline state '", pPipelineState->GetDesc().Name, "' "
                      "is not compatible with the pipeline state '", m_pPSO->GetDesc().Name, "' this SRB was created from and cannot be "
                      "used to initialize static resources.");
    }

    auto* pPSOVK = ValidatedCast<const PipelineStateVkImpl>(pPipelineState);
    pPSOVK->InitializeStaticSRBResources(m_ShaderResourceCache);
    m_bStaticResourcesInitialized = true;
}

}
