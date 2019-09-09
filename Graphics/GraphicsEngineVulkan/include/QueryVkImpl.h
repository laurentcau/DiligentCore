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
/// Declaration of Diligent::QueryVkImpl class

#include <deque>
#include "QueryVk.h"
#include "QueryBase.h"
#include "VulkanUtilities/VulkanQueryPool.h"
#include "RenderDeviceVkImpl.h"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Implementation of the Diligent::IQueryVk interface
class QueryVkImpl final : public QueryBase<IQueryVk, RenderDeviceVkImpl>
{
public:
    using TQueryBase = QueryBase<IQueryVk, RenderDeviceVkImpl>;

    QueryVkImpl(IReferenceCounters* pRefCounters,
                RenderDeviceVkImpl* pRendeDeviceVkImpl,
                const QueryDesc&    Desc,
                bool                IsDeviceInternal = false);
    ~QueryVkImpl();

    // Note that this method is not thread-safe. The reason is that VulkanQueryPool is not thread
    // safe, and DeviceContextVkImpl::SignalQuery() adds the Query to the pending Querys list that
    // are signaled later by the command context when it submits the command list. So there is no
    // guarantee that the Query pool is not accessed simultaneously by multiple threads even if the
    // Query object itself is protected by mutex.
    virtual Uint64 GetCompletedValue()override final;

    /// Resets the Query to the specified value. 
    virtual void Reset(Uint64 Value)override final;
    
    VulkanUtilities::QueryWrapper GetVkQuery() { return m_QueryPool.GetQuery(); }
    void AddPendingQuery(VulkanUtilities::QueryWrapper&& vkQuery, Uint64 QueryValue)
    {
        m_PendingQuerys.emplace_back(QueryValue, std::move(vkQuery));
    }

    void Wait();

private:
    VulkanUtilities::VulkanQueryPool m_QueryPool;
    std::deque<std::pair<Uint64, VulkanUtilities::QueryWrapper>> m_PendingQuerys;
    volatile Uint64 m_LastCompletedQueryValue = 0;
};

}
