/*******************************************************************************
Copyright (c) 2005-2009 David Williams

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution. 	
*******************************************************************************/

#ifndef __PolyVox_Compressor_H__
#define __PolyVox_Compressor_H__

#include "PolyVoxCore/Impl/TypeDef.h"

namespace PolyVox
{
	class Compressor
	{
	public:
		Compressor() {};
		virtual ~Compressor() {};

		// Computes a worst-case scenario for how big the output can be for a given input size. If
		// necessary you can use this as a destination buffer size, though it may be somewhat wasteful.
		virtual uint32_t getMaxCompressedSize(uint32_t uUncompressedInputSize) = 0;

		// Compresses the data.
		virtual uint32_t compress(void* pSrcData, uint32_t uSrcLength, void* pDstData, uint32_t uDstLength) = 0;

		// Decompresses the data.
		virtual uint32_t decompress(void* pSrcData, uint32_t uSrcLength, void* pDstData, uint32_t uDstLength) = 0;
	};
}

#endif //__PolyVox_Compressor_H__