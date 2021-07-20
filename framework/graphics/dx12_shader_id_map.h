
/*
** Copyright (c) 2021 LunarG, Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
*/

#ifndef GFXRECON_GRAPHICS_SHADER_ID_MAP_H
#define GFXRECON_GRAPHICS_SHADER_ID_MAP_H

#include "format/format.h"
#include "util/defines.h"

#include <map>
#include <array>
#include <d3d12.h>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(graphics)

typedef std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES> Dx12ShaderIdentifier;

class Dx12ShaderIdMap
{
  public:
    void Add(format::HandleId resource_id, uint8_t* old_shader_id, uint8_t* new_shader_id);

    void Remove(format::HandleId resource_id);

    // TODO: If it could give resource_id, it could reduce one iteration.
    //       It needs to wait until translating shader table done to see if it's possible.
    void Map(uint8_t** translated_shader_id) const;

  private:
    // Key is old shader id. Value is new.
    typedef std::map<Dx12ShaderIdentifier, Dx12ShaderIdentifier> Dx12ShaderIdentifierPair;
    typedef std::map<format::HandleId, Dx12ShaderIdentifierPair> ShaderIdMap;

  private:
    Dx12ShaderIdentifier PackDx12ShaderIdentifier(uint8_t* shader_id) const;
    ShaderIdMap          shader_id_map_;
};

GFXRECON_END_NAMESPACE(graphics)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif // GFXRECON_GRAPHICS_SHADER_ID_MAP_H