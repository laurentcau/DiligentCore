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
#include "RenderDeviceD3D12Impl.h"
#include "PipelineStateD3D12Impl.h"
#include "ShaderD3D12Impl.h"
#include "TextureD3D12Impl.h"
#include "DXGITypeConversions.h"
#include "SamplerD3D12Impl.h"
#include "BufferD3D12Impl.h"
#include "ShaderResourceBindingD3D12Impl.h"
#include "DeviceContextD3D12Impl.h"
#include "FenceD3D12Impl.h"
#include "QueryD3D12Impl.h"
#include "EngineMemory.h"
#include "d3dx12_win.h"

namespace Diligent
{

    std::function<void(HRESULT hr)> g_pCheckDeviceRemoved;

	void CheckDeviceRemoved(HRESULT hr)
	{
        if (g_pCheckDeviceRemoved)
            g_pCheckDeviceRemoved(hr);
	}

D3D_FEATURE_LEVEL RenderDeviceD3D12Impl :: GetD3DFeatureLevel() const
{
    D3D_FEATURE_LEVEL FeatureLevels[] = 
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevelsData = {};
    FeatureLevelsData.pFeatureLevelsRequested = FeatureLevels;
    FeatureLevelsData.NumFeatureLevels        = _countof(FeatureLevels);
    m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevelsData, sizeof(FeatureLevelsData));
    return FeatureLevelsData.MaxSupportedFeatureLevel;
}

RenderDeviceD3D12Impl :: RenderDeviceD3D12Impl(IReferenceCounters*           pRefCounters,
                                               IMemoryAllocator&             RawMemAllocator,
                                               IEngineFactory*               pEngineFactory,
                                               const EngineD3D12CreateInfo&  EngineCI,
                                               ID3D12Device*                 pd3d12Device,
                                               size_t                        CommandQueueCount,
                                               ICommandQueueD3D12**          ppCmdQueues) : 
    TRenderDeviceBase
    {
        pRefCounters,
        RawMemAllocator,
        pEngineFactory,
        CommandQueueCount,
        ppCmdQueues,
        EngineCI.NumDeferredContexts,
        DeviceObjectSizes
        {
            sizeof(TextureD3D12Impl),
            sizeof(TextureViewD3D12Impl),
            sizeof(BufferD3D12Impl),
            sizeof(BufferViewD3D12Impl),
            sizeof(ShaderD3D12Impl),
            sizeof(SamplerD3D12Impl),
            sizeof(PipelineStateD3D12Impl),
            sizeof(ShaderResourceBindingD3D12Impl),
            sizeof(FenceD3D12Impl),
			sizeof(QueryD3D12Impl)
        }
    },
    m_pd3d12Device  {pd3d12Device},
    m_EngineAttribs {EngineCI    },
    m_CmdListManager{*this       },
    m_CPUDescriptorHeaps
    {
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[2], D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         D3D12_DESCRIPTOR_HEAP_FLAG_NONE},
        {RawMemAllocator, *this, EngineCI.CPUDescriptorHeapAllocationSize[3], D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         D3D12_DESCRIPTOR_HEAP_FLAG_NONE}
    },
    m_GPUDescriptorHeaps
    {
        {RawMemAllocator, *this, EngineCI.GPUDescriptorHeapSize[0], EngineCI.GPUDescriptorHeapDynamicSize[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE},
        {RawMemAllocator, *this, EngineCI.GPUDescriptorHeapSize[1], EngineCI.GPUDescriptorHeapDynamicSize[1], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE}
    },
    m_ContextPool         {STD_ALLOCATOR_RAW_MEM(PooledCommandContext, GetRawAllocator(), "Allocator for vector<PooledCommandContext>")},
    m_DynamicMemoryManager{GetRawAllocator(), *this, EngineCI.NumDynamicHeapPagesToReserve, EngineCI.DynamicHeapPageSize},
    m_MipsGenerator       {pd3d12Device}
{
    m_DeviceCaps.DevType = DeviceType::D3D12;
    auto FeatureLevel = GetD3DFeatureLevel();
    switch (FeatureLevel)
    {
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_12_1:
            m_DeviceCaps.MajorVersion = 12;
            m_DeviceCaps.MinorVersion = FeatureLevel == D3D_FEATURE_LEVEL_12_1 ? 1 : 0;
            m_DeviceCaps.bBindlessSupported = true;
        break;

        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
            m_DeviceCaps.MajorVersion = 11;
            m_DeviceCaps.MinorVersion = FeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 1 : 0;
        break;
        
        case D3D_FEATURE_LEVEL_10_0:
        case D3D_FEATURE_LEVEL_10_1:
            m_DeviceCaps.MajorVersion = 10;
            m_DeviceCaps.MinorVersion = FeatureLevel == D3D_FEATURE_LEVEL_10_1 ? 1 : 0;
        break;

        default:
            UNEXPECTED("Unexpected D3D feature level");
    }
    m_DeviceCaps.bSeparableProgramSupported              = True;
    m_DeviceCaps.bMultithreadedResourceCreationSupported = True;

    // Direct3D12 supports shader model 5.1 on all feature levels (even on 11.0),
    // so bindless resources are always available.
    // https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-feature-levels#feature-level-support
    m_DeviceCaps.bBindlessSupported = True;

	m_MaxframeCount = EngineCI.MaxFrameCount;
	static_assert(std::tuple_size<decltype(m_QueryData)>::value == _countof(EngineCI.MaxQueryCountPerFrame), "");
	for (int i = 0; i < m_QueryData.size(); ++i)
		m_QueryData[i].m_QueryMaxCount = EngineCI.MaxQueryCountPerFrame[i];
	CreateQueryHeaps();
    
    g_pCheckDeviceRemoved = [this](HRESULT hr)
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            CComPtr<ID3D12DeviceRemovedExtendedData> pDred;
            hr = m_pd3d12Device->QueryInterface(IID_PPV_ARGS(&pDred));
            if (!FAILED(hr))
            {
                D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput;
                D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;
                HRESULT hrBc = pDred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput);
                HRESULT hrFault = pDred->GetPageFaultAllocationOutput(&DredPageFaultOutput);
                static const wchar_t* opNames[] = {
                    L"SETMARKER",
                    L"BEGINEVENT",
                    L"ENDEVENT",
                    L"DRAWINSTANCED",
                    L"DRAWINDEXEDINSTANCED",
                    L"EXECUTEINDIRECT",
                    L"DISPATCH",
                    L"COPYBUFFERREGION",
                    L"COPYTEXTUREREGION",
                    L"COPYRESOURCE",
                    L"COPYTILES",
                    L"RESOLVESUBRESOURCE",
                    L"CLEARRENDERTARGETVIEW",
                    L"CLEARUNORDEREDACCESSVIEW",
                    L"CLEARDEPTHSTENCILVIEW",
                    L"RESOURCEBARRIER",
                    L"EXECUTEBUNDLE",
                    L"PRESENT",
                    L"RESOLVEQUERYDATA",
                    L"BEGINSUBMISSION",
                    L"ENDSUBMISSION",
                    L"DECODEFRAME",
                    L"PROCESSFRAMES",
                    L"ATOMICCOPYBUFFERUINT",
                    L"ATOMICCOPYBUFFERUINT64",
                    L"RESOLVESUBRESOURCEREGION",
                    L"WRITEBUFFERIMMEDIATE",
                    L"DECODEFRAME1",
                    L"SETPROTECTEDRESOURCESESSION",
                    L"DECODEFRAME2",
                    L"PROCESSFRAMES1",
                    L"BUILDRAYTRACINGACCELERATIONSTRUCTURE",
                    L"EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO",
                    L"COPYRAYTRACINGACCELERATIONSTRUCTURE",
                    L"DISPATCHRAYS",
                    L"INITIALIZEMETACOMMAND",
                    L"EXECUTEMETACOMMAND",
                    L"ESTIMATEMOTION",
                    L"RESOLVEMOTIONVECTORHEAP",
                    L"SETPIPELINESTATE1",
                    L"INITIALIZEEXTENSIONCOMMAND",
                    L"EXECUTEEXTENSIONCOMMAND"
                };

                if (!FAILED(hrBc))
                {
                    const D3D12_AUTO_BREADCRUMB_NODE* pNode = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
                    int counter = 0;
                    while (pNode)
                    {
                        std::wstringstream s;
                        s << counter;
                        s << L" CmdList:" << (pNode->pCommandListDebugNameW ? pNode->pCommandListDebugNameW : L"<>");
                        s << L" CmdDebugName:" << (pNode->pCommandQueueDebugNameW ? pNode->pCommandQueueDebugNameW : L"<>");
                        s << L" CmdOp:";
                        for (UINT32 i = 0; i < pNode->BreadcrumbCount; ++i)
                        {
                            int op = pNode->pCommandHistory[i];
                            if (op < _countof(opNames))
                                s << opNames[op] << ", ";
                        }

                        s << "\n";
                        OutputDebugString(s.str().c_str());
                        pNode = pNode->pNext;
                    }
                }

                if (!FAILED(hrFault))
                {
                    {
                        std::wstringstream s;
                        s << "PageFault:";
                        s << "0x" << std::uppercase << std::setfill(L'0') << std::setw(16) << std::hex << DredPageFaultOutput.PageFaultVA;
                        s << "\n";
                        OutputDebugString(s.str().c_str());
                    }

                    static std::map<int, std::wstring> allocType =
                    {
                        {19,L"COMMAND_QUEUE"},
                        {20,L"COMMAND_ALLOCATOR"},
                        {21,L"PIPELINE_STATE"},
                        {22,L"COMMAND_LIST"},
                        {23,L"FENCE"},
                        {24,L"DESCRIPTOR_HEAP"},
                        {25,L"HEAP"},
                        {27,L"QUERY_HEAP"},
                        {28,L"COMMAND_SIGNATURE"},
                        {29,L"PIPELINE_LIBRARY"},
                        {30,L"VIDEO_DECODER"},
                        {32,L"VIDEO_PROCESSOR"},
                        {34,L"RESOURCE"},
                        {35,L"PASS"},
                        {36,L"CRYPTOSESSION"},
                        {37,L"CRYPTOSESSIONPOLICY"},
                        {38,L"PROTECTEDRESOURCESESSION"},
                        {39,L"VIDEO_DECODER_HEAP"},
                        {40,L"COMMAND_POOL"},
                        {41,L"COMMAND_RECORDER"},
                        {42,L"STATE_OBJECT"},
                        {43,L"METACOMMAND"},
                        {44,L"SCHEDULINGGROUP"},
                        {45,L"VIDEO_MOTION_ESTIMATOR"},
                        {46,L"VIDEO_MOTION_VECTOR_HEAP"},
                        {47,L"VIDEO_EXTENSION_COMMAND"}
                    };



                    {
                        OutputDebugString(L"RecentFreedAllocation:\n");
                        const D3D12_DRED_ALLOCATION_NODE* pNode = DredPageFaultOutput.pHeadRecentFreedAllocationNode;
                        int counter = 0;
                        while (pNode)
                        {
                            std::wstringstream s;
                            s << counter;
                            s << L" AllocName:" << (pNode->ObjectNameW ? pNode->ObjectNameW : L"<>");
                            auto it = allocType.find(pNode->AllocationType);
                            if (it != allocType.end())
                                s << L" AllocType:" << it->second;
                            s << "\n";
                            OutputDebugString(s.str().c_str());
                            pNode = pNode->pNext;
                        }
                    }


                    {
                        OutputDebugString(L"ExistingAllocation:\n");
                        const D3D12_DRED_ALLOCATION_NODE* pNode = DredPageFaultOutput.pHeadExistingAllocationNode;
                        int counter = 0;
                        while (pNode)
                        {
                            std::wstringstream s;
                            s << counter;
                            s << L" AllocName:" << pNode->ObjectNameW ? pNode->ObjectNameW : L"<>";
                            auto it = allocType.find(pNode->AllocationType);
                            if (it != allocType.end())
                                s << L" AllocType:" << it->second;
                            s << "\n";
                            OutputDebugString(s.str().c_str());
                            pNode = pNode->pNext;
                        }
                    }
                }
                DebugBreak();
            }
		}
    };

}



RenderDeviceD3D12Impl::~RenderDeviceD3D12Impl()
{
    // Wait for the GPU to complete all its operations
    IdleGPU();
    ReleaseStaleResources(true);

#ifdef DEVELOPMENT
    for (auto i=0; i < _countof(m_CPUDescriptorHeaps); ++i)
    {
        DEV_CHECK_ERR(m_CPUDescriptorHeaps[i].DvpGetTotalAllocationCount() == 0, "All CPU descriptor heap allocations must be released");
    }
    for (auto i=0; i < _countof(m_GPUDescriptorHeaps); ++i)
    {
        DEV_CHECK_ERR(m_GPUDescriptorHeaps[i].DvpGetTotalAllocationCount() == 0, "All GPU descriptor heap allocations must be released");
    }
#endif

    DEV_CHECK_ERR(m_DynamicMemoryManager.GetAllocatedPageCounter() == 0, "All allocated dynamic pages must have been returned to the manager at this point.");
    m_DynamicMemoryManager.Destroy();
    DEV_CHECK_ERR(m_CmdListManager.GetAllocatorCounter() == 0, "All allocators must have been returned to the manager at this point.");
    DEV_CHECK_ERR(m_AllocatedCtxCounter == 0, "All contexts must have been released.");

	m_ContextPool.clear();
    DestroyCommandQueues();
}

void RenderDeviceD3D12Impl::DisposeCommandContext(PooledCommandContext&& Ctx)
{
	CComPtr<ID3D12CommandAllocator> pAllocator; 
    Ctx->Close(pAllocator);
    // Since allocator has not been used, we cmd list manager can put it directly into the free allocator list
    m_CmdListManager.FreeAllocator(std::move(pAllocator));
    FreeCommandContext(std::move(Ctx));
}

void RenderDeviceD3D12Impl::FreeCommandContext(PooledCommandContext&& Ctx)
{
	std::lock_guard<std::mutex> LockGuard(m_ContextPoolMutex);
    m_ContextPool.emplace_back(std::move(Ctx));
#ifdef DEVELOPMENT
    Atomics::AtomicDecrement(m_AllocatedCtxCounter);
#endif
}

void RenderDeviceD3D12Impl::CloseAndExecuteTransientCommandContext(Uint32 CommandQueueIndex, PooledCommandContext&& Ctx)
{
    CComPtr<ID3D12CommandAllocator> pAllocator;
    ID3D12GraphicsCommandList* pCmdList = Ctx->Close(pAllocator);
    Uint64 FenceValue = 0;
    // Execute command list directly through the queue to avoid interference with command list numbers in the queue
    LockCommandQueue(CommandQueueIndex, 
        [&](ICommandQueueD3D12* pCmdQueue)
        {
            FenceValue = pCmdQueue->Submit(pCmdList);
        }
    );
	m_CmdListManager.ReleaseAllocator(std::move(pAllocator), CommandQueueIndex, FenceValue);
    FreeCommandContext(std::move(Ctx));
}

Uint64 RenderDeviceD3D12Impl::CloseAndExecuteCommandContext(Uint32 QueueIndex, PooledCommandContext&& Ctx, bool DiscardStaleObjects, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence> > >* pSignalFences)
{
    CComPtr<ID3D12CommandAllocator> pAllocator;
    ID3D12GraphicsCommandList* pCmdList = Ctx->Close(pAllocator);

    Uint64 FenceValue = 0;
    {
        // Stale objects should only be discarded when submitting cmd list from 
        // the immediate context, otherwise the basic requirement may be violated
        // as in the following scenario
        //                                                           
        //  Signaled        |                                        |
        //  Fence Value     |        Immediate Context               |            InitContext            |
        //                  |                                        |                                   |
        //    N             |  Draw(ResourceX)                       |                                   |
        //                  |  Release(ResourceX)                    |                                   |
        //                  |   - (ResourceX, N) -> Release Queue    |                                   |
        //                  |                                        | CopyResource()                    |
        //   N+1            |                                        | CloseAndExecuteCommandContext()   |
        //                  |                                        |                                   |
        //   N+2            |  CloseAndExecuteCommandContext()       |                                   |
        //                  |   - Cmd list is submitted with number  |                                   |
        //                  |     N+1, but resource it references    |                                   |
        //                  |     was added to the delete queue      |                                   |
        //                  |     with number N                      |                                   |
        auto SubmittedCmdBuffInfo = TRenderDeviceBase::SubmitCommandBuffer(QueueIndex, pCmdList, true);
        FenceValue = SubmittedCmdBuffInfo.FenceValue;
        if (pSignalFences != nullptr)
            SignalFences(QueueIndex, *pSignalFences);
    }

	m_CmdListManager.ReleaseAllocator(std::move(pAllocator), QueueIndex, FenceValue);
    FreeCommandContext(std::move(Ctx));

    PurgeReleaseQueue(QueueIndex);

    return FenceValue;
}

void RenderDeviceD3D12Impl::SignalFences(Uint32 QueueIndex, std::vector<std::pair<Uint64, RefCntAutoPtr<IFence> > >& SignalFences)
{
    for (auto& val_fence : SignalFences)
    {
        auto* pFenceD3D12Impl = val_fence.second.RawPtr<FenceD3D12Impl>();
        auto* pd3d12Fence = pFenceD3D12Impl->GetD3D12Fence();
        m_CommandQueues[QueueIndex].CmdQueue->SignalFence(pd3d12Fence, val_fence.first);
    }
}

void RenderDeviceD3D12Impl::IdleGPU() 
{ 
    IdleAllCommandQueues(true);
    ReleaseStaleResources();
}

void RenderDeviceD3D12Impl::FlushStaleResources(Uint32 CmdQueueIndex)
{
    // Submit empty command list to the queue. This will effectively signal the fence and 
    // discard all resources
    ID3D12GraphicsCommandList* pNullCmdList = nullptr;
    TRenderDeviceBase::SubmitCommandBuffer(CmdQueueIndex, pNullCmdList, true);
}

void RenderDeviceD3D12Impl::ReleaseStaleResources(bool ForceRelease)
{
    PurgeReleaseQueues(ForceRelease);
}


RenderDeviceD3D12Impl::PooledCommandContext RenderDeviceD3D12Impl::AllocateCommandContext(const Char* ID)
{
    {
    	std::lock_guard<std::mutex> LockGuard(m_ContextPoolMutex);
        if (!m_ContextPool.empty())
        {
            PooledCommandContext Ctx = std::move(m_ContextPool.back());
            m_ContextPool.pop_back();
            Ctx->Reset(m_CmdListManager);
            Ctx->SetID(ID);
#ifdef DEVELOPMENT
            Atomics::AtomicIncrement(m_AllocatedCtxCounter);
#endif
            return Ctx;
        }
    }

    auto& CmdCtxAllocator = GetRawAllocator();
    auto* pRawMem = ALLOCATE(CmdCtxAllocator, "CommandContext instance", CommandContext, 1);
	auto pCtx = new (pRawMem) CommandContext(m_CmdListManager);
    pCtx->SetID(ID);
#ifdef DEVELOPMENT
    Atomics::AtomicIncrement(m_AllocatedCtxCounter);
#endif
    return PooledCommandContext(pCtx, CmdCtxAllocator);
}


void RenderDeviceD3D12Impl::TestTextureFormat( TEXTURE_FORMAT TexFormat )
{
    auto &TexFormatInfo = m_TextureFormatsInfo[TexFormat];
    VERIFY( TexFormatInfo.Supported, "Texture format is not supported" );

    auto DXGIFormat = TexFormatToDXGI_Format(TexFormat);

    D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = {DXGIFormat};
    auto hr = m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
    if (FAILED(hr))
    {
        LOG_ERROR_MESSAGE("CheckFormatSupport() failed for format ", DXGIFormat);
        return;
    }

    TexFormatInfo.Filterable      = ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0) || 
                                    ((FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON) != 0);
    TexFormatInfo.ColorRenderable = (FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0;
    TexFormatInfo.DepthRenderable = (FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0;
    TexFormatInfo.Tex1DFmt        = (FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE1D) != 0;
    TexFormatInfo.Tex2DFmt        = (FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0;
    TexFormatInfo.Tex3DFmt        = (FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE3D) != 0;
    TexFormatInfo.TexCubeFmt      = (FormatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE) != 0;

    TexFormatInfo.SampleCounts = 0x0;
    for(Uint32 SampleCount = 1; SampleCount <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT; SampleCount *= 2)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels = {DXGIFormat, SampleCount};
        hr = m_pd3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &QualityLevels, sizeof(QualityLevels));
        if(SUCCEEDED(hr) && QualityLevels.NumQualityLevels > 0)
            TexFormatInfo.SampleCounts |= SampleCount;
    }
}


IMPLEMENT_QUERY_INTERFACE( RenderDeviceD3D12Impl, IID_RenderDeviceD3D12, TRenderDeviceBase )

void RenderDeviceD3D12Impl::CreatePipelineState(const PipelineStateDesc& PipelineDesc, IPipelineState** ppPipelineState)
{
    CreateDeviceObject("Pipeline State", PipelineDesc, ppPipelineState, 
        [&]()
        {
            PipelineStateD3D12Impl *pPipelineStateD3D12( NEW_RC_OBJ(m_PSOAllocator, "PipelineStateD3D12Impl instance", PipelineStateD3D12Impl)(this, PipelineDesc ) );
            pPipelineStateD3D12->QueryInterface( IID_PipelineState, reinterpret_cast<IObject**>(ppPipelineState) );
            OnCreateDeviceObject( pPipelineStateD3D12 );
        } 
    );
}

void RenderDeviceD3D12Impl :: CreateBufferFromD3DResource(ID3D12Resource* pd3d12Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)
{
    CreateDeviceObject("buffer", BuffDesc, ppBuffer, 
        [&]()
        {
            BufferD3D12Impl *pBufferD3D12( NEW_RC_OBJ(m_BufObjAllocator, "BufferD3D12Impl instance", BufferD3D12Impl)(m_BuffViewObjAllocator, this, BuffDesc, InitialState, pd3d12Buffer ) );
            pBufferD3D12->QueryInterface( IID_Buffer, reinterpret_cast<IObject**>(ppBuffer) );
            pBufferD3D12->CreateDefaultViews();
            OnCreateDeviceObject( pBufferD3D12 );
        } 
    );
}

void RenderDeviceD3D12Impl :: CreateBuffer(const BufferDesc& BuffDesc, const BufferData* pBuffData, IBuffer** ppBuffer)
{
    CreateDeviceObject("buffer", BuffDesc, ppBuffer, 
        [&]()
        {
            BufferD3D12Impl *pBufferD3D12( NEW_RC_OBJ(m_BufObjAllocator, "BufferD3D12Impl instance", BufferD3D12Impl)(m_BuffViewObjAllocator, this, BuffDesc, pBuffData ) );
            pBufferD3D12->QueryInterface( IID_Buffer, reinterpret_cast<IObject**>(ppBuffer) );
            pBufferD3D12->CreateDefaultViews();
            OnCreateDeviceObject( pBufferD3D12 );
        } 
    );
}


void RenderDeviceD3D12Impl :: CreateShader(const ShaderCreateInfo& ShaderCI, IShader** ppShader)
{
    CreateDeviceObject( "shader", ShaderCI.Desc, ppShader, 
        [&]()
        {
            ShaderD3D12Impl *pShaderD3D12( NEW_RC_OBJ(m_ShaderObjAllocator, "ShaderD3D12Impl instance", ShaderD3D12Impl)(this, ShaderCI ) );
            pShaderD3D12->QueryInterface( IID_Shader, reinterpret_cast<IObject**>(ppShader) );

            OnCreateDeviceObject( pShaderD3D12 );
        } 
    );
}

void RenderDeviceD3D12Impl::CreateTextureFromD3DResource(ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, ITexture** ppTexture)
{
    TextureDesc TexDesc;
    TexDesc.Name = "Texture from d3d12 resource";
    CreateDeviceObject( "texture", TexDesc, ppTexture, 
        [&]()
        {
            TextureD3D12Impl *pTextureD3D12 = NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, InitialState, pd3d12Texture);

            pTextureD3D12->QueryInterface( IID_Texture, reinterpret_cast<IObject**>(ppTexture) );
            pTextureD3D12->CreateDefaultViews();
            OnCreateDeviceObject( pTextureD3D12 );
        } 
    );
}

void RenderDeviceD3D12Impl::CreateTexture(const TextureDesc& TexDesc, ID3D12Resource* pd3d12Texture, RESOURCE_STATE InitialState, TextureD3D12Impl** ppTexture)
{
    CreateDeviceObject( "texture", TexDesc, ppTexture, 
        [&]()
        {
            TextureD3D12Impl* pTextureD3D12 = NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, InitialState, pd3d12Texture);
            pTextureD3D12->QueryInterface( IID_TextureD3D12, reinterpret_cast<IObject**>(ppTexture) );
        }
    );
}

void RenderDeviceD3D12Impl :: CreateTexture(const TextureDesc& TexDesc, const TextureData* pData, ITexture** ppTexture)
{
    CreateDeviceObject( "texture", TexDesc, ppTexture, 
        [&]()
        {
            TextureD3D12Impl* pTextureD3D12 = NEW_RC_OBJ(m_TexObjAllocator, "TextureD3D12Impl instance", TextureD3D12Impl)(m_TexViewObjAllocator, this, TexDesc, pData );

            pTextureD3D12->QueryInterface( IID_Texture, reinterpret_cast<IObject**>(ppTexture) );
            pTextureD3D12->CreateDefaultViews();
            OnCreateDeviceObject( pTextureD3D12 );
        } 
    );
}

void RenderDeviceD3D12Impl :: CreateSampler(const SamplerDesc& SamplerDesc, ISampler** ppSampler)
{
    CreateDeviceObject( "sampler", SamplerDesc, ppSampler, 
        [&]()
        {
            m_SamplersRegistry.Find( SamplerDesc, reinterpret_cast<IDeviceObject**>(ppSampler) );
            if( *ppSampler == nullptr )
            {
                SamplerD3D12Impl *pSamplerD3D12( NEW_RC_OBJ(m_SamplerObjAllocator, "SamplerD3D12Impl instance", SamplerD3D12Impl)(this, SamplerDesc ) );
                pSamplerD3D12->QueryInterface( IID_Sampler, reinterpret_cast<IObject**>(ppSampler) );
                OnCreateDeviceObject( pSamplerD3D12 );
                m_SamplersRegistry.Add( SamplerDesc, *ppSampler );
            }
        }
    );
}

void RenderDeviceD3D12Impl::CreateFence(const FenceDesc& Desc, IFence** ppFence)
{
    CreateDeviceObject( "Fence", Desc, ppFence, 
        [&]()
        {
            FenceD3D12Impl* pFenceD3D12( NEW_RC_OBJ(m_FenceAllocator, "FenceD3D12Impl instance", FenceD3D12Impl)
                                                   (this, Desc) );
            pFenceD3D12->QueryInterface( IID_Fence, reinterpret_cast<IObject**>(ppFence) );
            OnCreateDeviceObject( pFenceD3D12 );
        }
    );
}

DescriptorHeapAllocation RenderDeviceD3D12Impl :: AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
    VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Invalid heap type");
    return m_CPUDescriptorHeaps[Type].Allocate(Count);
}

DescriptorHeapAllocation RenderDeviceD3D12Impl :: AllocateGPUDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count /*= 1*/)
{
    VERIFY(Type >= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && Type <= D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Invalid heap type");
    return m_GPUDescriptorHeaps[Type].Allocate(Count);
}

void RenderDeviceD3D12Impl::CreateQuery(const QueryDesc& Desc, IQuery** ppQuery)
{
	CreateDeviceObject("Query", Desc, ppQuery,
		[&]()
		{
			QueryD3D12Impl* pQueryD3D12(NEW_RC_OBJ(m_QueryAllocator, "QueryD3D12Impl instance", QueryD3D12Impl)
				(this, Desc));
			pQueryD3D12->QueryInterface(IID_Query, reinterpret_cast<IObject**>(ppQuery));
			OnCreateDeviceObject(pQueryD3D12);
		}
	);
}

void RenderDeviceD3D12Impl::CreateQueryHeaps()
{
	const UINT maxFrameCount = 2;

	for (int i = 0; i < m_QueryData.size(); ++i)
	{
		if (m_QueryData[i].m_QueryMaxCount != 0)
		{
			const UINT resultCount = m_QueryData[i].m_QueryMaxCount * (maxFrameCount + 1);
			const UINT resultBufferSize = resultCount * sizeof(UINT64);

			D3D12_QUERY_HEAP_DESC timestampHeapDesc = {};
			timestampHeapDesc.Type = i == D3D12_QUERY_TYPE_TIMESTAMP ? D3D12_QUERY_HEAP_TYPE_TIMESTAMP : D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			timestampHeapDesc.Count = resultCount;

			CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_READBACK);
			CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(resultBufferSize);

			m_pd3d12Device->CreateCommittedResource(
				&heapProp,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_QueryData[i].m_QueryResultBuffers)
			);

			m_pd3d12Device->CreateQueryHeap(&timestampHeapDesc, IID_PPV_ARGS(&m_QueryData[i].m_QueryHeap));
		}
	}

}

}
