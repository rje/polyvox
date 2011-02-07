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

#ifndef __PolyVox_Block_H__
#define __PolyVox_Block_H__

#include "PolyVoxForwardDeclarations.h"

#include <vector>

namespace PolyVox
{
	template <typename VoxelType>
	class Block
	{
		//Make VolumeSampler a friend
		friend class VolumeSampler<VoxelType>;
	public:
		Block(uint16_t uSideLength = 0);
		Block(const Block& rhs);
		~Block();

		Block& operator=(const Block& rhs);

		uint16_t getSideLength(void) const;
		VoxelType getVoxelAt(uint16_t uXPos, uint16_t uYPos, uint16_t uZPos) const;
		VoxelType getVoxelAt(const Vector3DUint16& v3dPos) const;

		void setVoxelAt(uint16_t uXPos, uint16_t uYPos, uint16_t uZPos, VoxelType tValue);
		void setVoxelAt(const Vector3DUint16& v3dPos, VoxelType tValue);

		void fill(VoxelType tValue);
		void resize(uint16_t uSideLength);
		uint32_t sizeInChars(void);

	public:
		void compress(void);
		void uncompress(void);

		uint16_t m_uSideLength;
		uint8_t m_uSideLengthPower;	
		VoxelType* m_tUncompressedData;
		bool m_bIsCompressed;
		bool m_bIsUncompressedDataModified;
		uint64_t m_uTimestamp;

		std::vector<uint8_t> runlengths;
		std::vector<VoxelType> values;
	};
}

#include "Block.inl"

#endif
