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

#include "QueryGLImpl.h"
#include "EngineMemory.h"

namespace Diligent
{
    
QueryGLImpl :: QueryGLImpl(IReferenceCounters* pRefCounters,
                           RenderDeviceGLImpl* pDevice,
                           const QueryDesc&    Desc) : 
    TQueryBase(pRefCounters, pDevice, Desc)
{
	glGenQueries(1, &m_QueryId);
	switch (Desc.Type)
	{
	case QUERY_TYPE_OCCLUSION: m_glType = GL_SAMPLES_PASSED_ARB; break;
	case QUERY_TYPE_BINARY_OCCLUSION: m_glType = GL_ANY_SAMPLES_PASSED; break;
	case QUERY_TYPE_TIMESTAMP: m_glType = GL_TIMESTAMP; break;
	default:
		break;
	}
}

QueryGLImpl :: ~QueryGLImpl()
{
}

void QueryGLImpl::Begin()
{
	if (m_glType != GL_TIMESTAMP)
	{
		glBeginQuery(m_glType, m_QueryId);
		CHECK_GL_ERROR("glBeginQuery() failed");
	}
}

void QueryGLImpl::End()
{
	if (m_glType == GL_TIMESTAMP)
	{
		glQueryCounter(m_QueryId, GL_TIMESTAMP);
		CHECK_GL_ERROR("glQueryCounter() failed");
	}
	else
	{
		glEndQuery(m_glType);
		CHECK_GL_ERROR("glEndQuery() failed");
	}
}

QUERY_STATUS QueryGLImpl::GetValue(Uint64& v) const
{
	GLuint rs;
	glGetQueryObjectuiv(m_QueryId, GL_QUERY_RESULT_AVAILABLE_ARB, &rs);
	CHECK_GL_ERROR("glGetQueryObjectuiv() failed");
	if (rs == GL_FALSE)
	{
		return QUERY_STATUS_RETRY;
	}
	glGetQueryObjectuiv(m_QueryId, GL_QUERY_RESULT_ARB, &rs);
	CHECK_GL_ERROR("glGetQueryObjectuiv() failed");
	v = rs;
	return QUERY_STATUS_SUCESS;
}


}
