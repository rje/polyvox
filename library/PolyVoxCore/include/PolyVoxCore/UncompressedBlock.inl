/*******************************************************************************
Copyright (c) 2005-2013 David Williams and Matthew Williams

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

namespace PolyVox
{
	template <typename VoxelType>
	UncompressedBlock<VoxelType>::UncompressedBlock(uint16_t uSideLength)
		:m_tData(0)
		,m_uSideLength(0)
		,m_uSideLengthPower(0)
	{
		// Compute the side length               
		m_uSideLength = uSideLength;
		m_uSideLengthPower = logBase2(uSideLength);

		// Allocate the data
		const uint32_t uNoOfVoxels = m_uSideLength * m_uSideLength * m_uSideLength;
		m_tData = new VoxelType[uNoOfVoxels];               
	}

	template <typename VoxelType>
	UncompressedBlock<VoxelType>::~UncompressedBlock()
	{
		delete[] m_tData;
		m_tData = 0;
	}

	template <typename VoxelType>
	VoxelType* UncompressedBlock<VoxelType>::getData(void) const
	{
		return m_tData;
	}

	template <typename VoxelType>
	uint32_t UncompressedBlock<VoxelType>::getDataSizeInBytes(void) const
	{
		return m_uSideLength * m_uSideLength * m_uSideLength * sizeof(VoxelType);
	}

	template <typename VoxelType>
	VoxelType UncompressedBlock<VoxelType>::getVoxel(uint16_t uXPos, uint16_t uYPos, uint16_t uZPos) const
	{
		// This code is not usually expected to be called by the user, with the exception of when implementing paging 
		// of uncompressed data. It's a performance critical code path so  we use asserts rather than exceptions.
		POLYVOX_ASSERT(uXPos < m_uSideLength, "Supplied position is outside of the block");
		POLYVOX_ASSERT(uYPos < m_uSideLength, "Supplied position is outside of the block");
		POLYVOX_ASSERT(uZPos < m_uSideLength, "Supplied position is outside of the block");
		POLYVOX_ASSERT(m_tData, "No uncompressed data - block must be decompressed before accessing voxels.");

		return m_tData
			[
				uXPos + 
				uYPos * m_uSideLength + 
				uZPos * m_uSideLength * m_uSideLength
			];
	}

	template <typename VoxelType>
	VoxelType UncompressedBlock<VoxelType>::getVoxel(const Vector3DUint16& v3dPos) const
	{
		return getVoxel(v3dPos.getX(), v3dPos.getY(), v3dPos.getZ());
	}

	template <typename VoxelType>
	void UncompressedBlock<VoxelType>::setVoxelAt(uint16_t uXPos, uint16_t uYPos, uint16_t uZPos, VoxelType tValue)
	{
		// This code is not usually expected to be called by the user, with the exception of when implementing paging 
		// of uncompressed data. It's a performance critical code path so  we use asserts rather than exceptions.
		POLYVOX_ASSERT(uXPos < m_uSideLength, "Supplied position is outside of the block");
		POLYVOX_ASSERT(uYPos < m_uSideLength, "Supplied position is outside of the block");
		POLYVOX_ASSERT(uZPos < m_uSideLength, "Supplied position is outside of the block");
		POLYVOX_ASSERT(m_tData, "No uncompressed data - block must be decompressed before accessing voxels.");

		m_tData
			[
				uXPos + 
				uYPos * m_uSideLength + 
				uZPos * m_uSideLength * m_uSideLength
			] = tValue;

		this->m_bDataModified = true;
	}

	template <typename VoxelType>
    void UncompressedBlock<VoxelType>::setVoxelAt(const Vector3DUint16& v3dPos, VoxelType tValue)
    {
		setVoxelAt(v3dPos.getX(), v3dPos.getY(), v3dPos.getZ(), tValue);
    }

	template <typename VoxelType>
	uint32_t UncompressedBlock<VoxelType>::calculateSizeInBytes(void)
	{
		// Returns the size of this class plus the size of the uncompressed data.
		uint32_t uSizeInBytes = sizeof(UncompressedBlock<VoxelType>) + (m_uSideLength * m_uSideLength * m_uSideLength * sizeof(VoxelType));
		return  uSizeInBytes;
	}
}