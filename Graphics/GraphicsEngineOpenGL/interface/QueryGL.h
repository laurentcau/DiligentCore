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
/// Definition of the Diligent::IQueryGL interface

#include "../../GraphicsEngine/interface/Query.h"

namespace Diligent
{

// {F83DD855-5D7F-40F4-BEE5-62A09978912B}
static constexpr INTERFACE_ID IID_QueryGL =
{ 0xf83dd855, 0x5d7f, 0x40f4, { 0xbe, 0xe5, 0x62, 0xa0, 0x99, 0x78, 0x91, 0x2b } };


/// Interface to the Query object implemented in GL
class IQueryGL : public IQuery
{
public:
    /// Returns OpenGL sync object
    //virtual IGLQuery* GetGLQuery() = 0;
};

}
