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
#include "CommandListManager.h"
#include "RenderDeviceD3D12Impl.h"

namespace Diligent
{

CommandListManager::CommandListManager(RenderDeviceD3D12Impl& DeviceD3D12Impl) : 
    m_DeviceD3D12Impl{DeviceD3D12Impl},
    m_FreeAllocators {STD_ALLOCATOR_RAW_MEM(CComPtr<ID3D12CommandAllocator>, GetRawAllocator(), "Allocator for vector<CComPtr<ID3D12CommandAllocator>>")}
{
}

CommandListManager::~CommandListManager()
{
    DEV_CHECK_ERR(m_AllocatorCounter == 0, m_AllocatorCounter, " allocator(s) have not been returned to the manager. This will cause a crash if these allocators are referenced by release queues and later returned via FreeAllocator()");
    LOG_INFO_MESSAGE("Command list manager: created ", m_FreeAllocators.size(), " allocators");
}

void CommandListManager::CreateNewCommandList( ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator )
{
	RequestAllocator(Allocator);
    auto* pd3d12Device = m_DeviceD3D12Impl.GetD3D12Device();
	auto hr = pd3d12Device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, *Allocator, nullptr, __uuidof(*List), reinterpret_cast<void**>(List) );
    CHECK_D3D_RESULT_THROW(hr, "Failed to create command list");
	(*List)->SetName(L"CommandList");
}


void CommandListManager::RequestAllocator(ID3D12CommandAllocator** ppAllocator)
{
	std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    VERIFY( (*ppAllocator) == nullptr, "Allocator pointer is not null" );
    (*ppAllocator) = nullptr;

    if (!m_FreeAllocators.empty())
	{
		*ppAllocator = m_FreeAllocators.back().Detach();
		auto hr = (*ppAllocator)->Reset();
        CHECK_D3D_RESULT_THROW(hr, "Failed to reset command allocator");
		m_FreeAllocators.pop_back();
	}

	// If no allocators were ready to be reused, create a new one
	if ((*ppAllocator) == nullptr)
	{
        auto* pd3d12Device = m_DeviceD3D12Impl.GetD3D12Device();
		auto hr = pd3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(*ppAllocator), reinterpret_cast<void**>(ppAllocator));
        CHECK_D3D_RESULT_THROW(hr, "Failed to create command allocator");
		wchar_t AllocatorName[32];
        swprintf(AllocatorName, _countof(AllocatorName), L"Cmd list allocator %ld", Atomics::AtomicIncrement(m_NumAllocators)-1);
		(*ppAllocator)->SetName(AllocatorName);
	}
#ifdef DEVELOPMENT
    Atomics::AtomicIncrement(m_AllocatorCounter);
#endif
}

void CommandListManager::ReleaseAllocator( CComPtr<ID3D12CommandAllocator>&& Allocator, Uint32 CmdQueue, Uint64 FenceValue )
{
    struct StaleAllocator
    {
        CComPtr<ID3D12CommandAllocator> Allocator;
        CommandListManager*             Mgr;

        StaleAllocator(CComPtr<ID3D12CommandAllocator>&& _Allocator, CommandListManager& _Mgr)noexcept :
            Allocator (std::move(_Allocator)),
            Mgr       (&_Mgr)
        {
        }

        StaleAllocator            (const StaleAllocator&)  = delete;
        StaleAllocator& operator= (const StaleAllocator&)  = delete;
        StaleAllocator& operator= (      StaleAllocator&&) = delete;
            
        StaleAllocator(StaleAllocator&& rhs)noexcept : 
            Allocator (std::move(rhs.Allocator)),
            Mgr       (rhs.Mgr)
        {
            rhs.Mgr       = nullptr;
        }

        ~StaleAllocator()
        {
            if (Mgr != nullptr)
                Mgr->FreeAllocator( std::move(Allocator) );
        }
    };
    m_DeviceD3D12Impl.GetReleaseQueue(CmdQueue).DiscardResource(StaleAllocator{std::move(Allocator), *this}, FenceValue);
}

void CommandListManager::FreeAllocator( CComPtr<ID3D12CommandAllocator>&& Allocator )
{
	std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);
	m_FreeAllocators.emplace_back( std::move(Allocator) );
#ifdef DEVELOPMENT
    Atomics::AtomicDecrement(m_AllocatorCounter);
#endif
}

}
