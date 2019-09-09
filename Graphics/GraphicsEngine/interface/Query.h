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
/// Definition of the Diligent::IQuery interface and related data structures

#include "DeviceObject.h"

namespace Diligent
{

class IDeviceContext;

// {D36985CB-1F62-4C3F-9173-FDA56CE416D3}
static constexpr INTERFACE_ID IID_Query =
{ 0xd36985cb, 0x1f62, 0x4c3f, { 0x91, 0x73, 0xfd, 0xa5, 0x6c, 0xe4, 0x16, 0xd3 } };

enum QUERY_TYPE
{
	QUERY_TYPE_OCCLUSION,
	QUERY_TYPE_BINARY_OCCLUSION, //DX12 only, Opengl & DX11 use standard OCCLUSION
	QUERY_TYPE_TIMESTAMP
};

enum QUERY_STATUS
{
	QUERY_STATUS_RETRY,
	QUERY_STATUS_SUCESS
};

/// Query description
struct QueryDesc : DeviceObjectAttribs
{
	QUERY_TYPE Type;

  
	QueryDesc()noexcept{}

	QueryDesc(QUERY_TYPE _QueryType) :
		Type(_QueryType)
    {}

    /// Tests if two structures are equivalent

    /// \param [in] RHS - reference to the structure to perform comparison with
    /// \return 
    /// - True if all members of the two structures except for the Name are equal.
    /// - False otherwise.
    /// The operator ignores DeviceObjectAttribs::Name field as it does not affect 
    /// the texture description state.
	bool operator ==(const QueryDesc& RHS)const
    {
                // Name is primarily used for debug purposes and does not affect the state.
                // It is ignored in comparison operation.
		return  // strcmp(Name, RHS.Name) == 0          &&
			Type == RHS.Type;
    }
};


/// Query interface
class IQuery : public IDeviceObject
{
public:
    /// Queries the specific interface, see IObject::QueryInterface() for details
    virtual void QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)override = 0;

    /// Returns the query description used to create the object
    virtual const QueryDesc& GetDesc()const override = 0;
    
	virtual QUERY_STATUS GetValue(Uint64& v) const  = 0;
};

}
