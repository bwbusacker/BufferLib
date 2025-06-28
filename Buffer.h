//==============================================================================
//         Copyright 2022 Micron Technology, Inc. All Rights Reserved.
// 
//    THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE SECRETS OF 
//   MICRON TECHNOLOGY, INC. USE, DISCLOSURE, OR REPRODUCTION IS PROHIBITED 
//   WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF MICRON TECHNOLOGY, INC.
//==============================================================================

#pragma once
#ifndef _BUFFER_H_
#define _BUFFER_H_

//#include "dmx/DM3Errors.h"
#include "Errors.h"
#include "Random32.h"
//#include "dmx/Printable.h"
#include "Printable.h"
#include "TypeDefs.h"
#include "Utils.h"
#include "CompareResult.h"

#include <boost/thread/mutex.hpp>

#include <iostream>
#include <vector>
#include <string>

#define COMPRESSION_SIZE_PER_SECTOR 21
#define COMPRESSION_PATTERN_SIZE_IN_BYTE 12
#define COMPRESSION_LBA_SIZE_IN_BYTE 8
#define COMPRESSION_TYPE_SIZE_IN_BYTE 1
#define COMPRESSION_MAX_PATTERN_LEN 8

typedef enum
{
	eFixPattern,
	eIncrementingPattern,
	eDecrementingPattern,
	eRandomPattern,
}ECompressionType;

namespace boost
{
	template<class T> class shared_ptr;
}

namespace ufs
{
	/// <summary>
	/// Used in Buffer and Buffers classes.Specifies grouping
	/// format of printed buffer data.  Formatting of buffer
	/// data using the following format definitions separated
	/// by two spaces:
	/// #FORMAT
	///     Byte = 2 characters,
	///     Word = 4 characters, and
	///    Dword = 8 characters.
	/// #ENDFORMAT
	/// </summary>
	enum ByteGrouping
	{
		Byte = 2,
		Word = 4,
		DWord = 8
	};

	const size_t DEFAULT_BYTES_PER_SECTOR = 512;

	// Default sector count is the max value for the Count of sectors for the ATA commands. 0x10000 is the actual sector count
	// when zero is passed in for sector count for a 48-bit command like WriteDmaExt.
	const size_t DEFAULT_SECTOR_COUNT = 0x10000;

	class CompareResult;
	class Messenger;
	class Buffer;

	/// <summary>
	/// Provides a block of memory stored internally as an array of bytes. The
	/// default size is 32 megabytes - or 65,536 sectors (0x10000), each of 512
	/// (0x200) bytes - for a total of 33,554,432 (0x2000000) bytes - or 32 megabytes.
	///
	/// The concept of sector count and size is for doing media read and writes.
	/// Not all users of the Buffer class will need the concept of sector count
	/// or size. Managing TCG data would be an example of an application where
	/// sectors are not applicable, but modification of raw data is required.
	///
	/// Buffer has a variety of Fill methods (:class:`dmx.Buffer.FillRandom`,
	/// :class:`dmx.Buffer.FillZeros`), that can be used to populate the buffer
	/// with data. There are also methods to provide access at the bit, byte and
	/// word level (:class:`dmx.Buffer.GetByte`, :class:`dmx.Buffer.GetByteBit`,
	/// :class:`dmx.Buffer.GetWord`, :class:`dmx.Buffer.GetWordBit`) as well as
	/// methods to modify the data at the byte and word level (
	/// :class:`dmx.Buffer.SetByte`, :class:`dmx.Buffer.SetWord`). Methods are
	/// also available for saving a buffer to a file and loading a buffer from a
	/// file.
	///
	/// The :class:`dmx.Buffer.CompareTo` method is provided as a means to
	/// compare two buffers to see if they contain the same data, and return the
	/// details of that comparison via a :class:`dmx.CompareResult` object.
	///
	/// The :class:`dmx.Buffer.Resize` method is available to change the size of
	/// an existing buffer.
	///
	/// Keep in mind that each buffer that you create in code, or is
	/// created by default, consumes memory. If a script uses larger buffers
	/// (e.g. for PCIe drives) or multiple user buffers, it is possible to run
	/// out of system memory, particularly when running on a 32-bit OS, which
	/// provides access to less than 4GB of memory. If you need to use large
	/// buffers or many buffers, try to do so on a 64-bit OS with plenty of
	/// physical memory on your computer. It is also advisable to avoid keeping
	/// unneeded buffers, or resize the default buffers to a very small size if
	/// you do not need them.
	///
	/// </summary>
	class Buffer : public Printable
	{
	private: //Member variables
		//not implemented /////////////////////////////////////
		Buffer& operator=(const Buffer&); // not implemented //
		//not implemented /////////////////////////////////////

		std::string _name;
		UInt8* _data;
		UInt8* _dataStart;
		size_t _bytesPerSector;
		size_t _sectorCount;
		Random32 * _random;
		size_t _allocatedByteCount;
		size_t _dataBufferSize;
		bool _usePatternMode;

	private: // Private methods
		//inline size_t ValidateSectorRangeAndGetSectorCount(size_t startSector, size_t sectorCount) const;
		inline size_t ValidateSectorRangeAndGetSectorCount(size_t startSector, size_t sectorCount) const
		{
			sectorCount = sectorCount == 0 ? _sectorCount - startSector : sectorCount;

			if (startSector >= GetSectorCount())
			{
				throw ufs::OutOfRangeError("startSector must be less than SectorCount of buffer.");
			}

			if ((startSector + sectorCount) > GetSectorCount())
			{
				throw ufs::OutOfRangeError("startSector plus sectorCount must be less the SectorCount of buffer.");
			}

			return sectorCount;
		}


		//void GetStartAndStopBytesFromSectors(size_t startSector, size_t sectorCount, size_t& startByte, size_t& endByte) const;
		inline void GetStartAndStopBytesFromSectors(size_t startSector, size_t sectorCount, size_t& startByte, size_t& endByte) const
		{
			size_t newSectorCount = ValidateSectorRangeAndGetSectorCount(startSector, sectorCount);
			startByte = startSector * GetBytesPerSector();
			endByte = newSectorCount == 0 ? GetTotalBytes() : (startSector + newSectorCount) * GetBytesPerSector();
		}

		//inline size_t ValidateByteRangeAndGetLength(size_t startingOffset, size_t length) const;
		inline size_t ValidateByteRangeAndGetLength(size_t startingOffset, size_t length) const
		{
			length = length == 0 ? GetTotalBytes() - startingOffset : length;

			if (startingOffset >= GetTotalBytes())
			{
				throw ufs::OutOfRangeError("startingOffset (" + std::to_string(startingOffset)
					+ ") must be less than the total number of bytes in the buffer (" 
					+ std::to_string(GetTotalBytes()) + ").");
			}

			if ((startingOffset + length) > GetTotalBytes())
			{
				throw ufs::OutOfRangeError("startingOffset plus length must be less that the number of bytes in the buffer.");
			}

			return length;
		}

		//inline Random32 * GetRandom(bool useSeed, UInt32 seed = 0);
		inline ufs::Random32 * GetRandom(bool useSeed, UInt32 seed)
		{
			if (_random == NULL)
			{
				if (useSeed)
				{
					_random = new ufs::Random32(seed);
				}
				else
				{
					_random = new ufs::Random32();
				}
			}
			else
			{
				if (useSeed)
				{
					_random->Seed(seed);
				}
				else
				{
					if (_random->GetIsSeeded())
					{
						_random->Seed(0);
					}
				}
			}

			return _random;
		}

		Buffer& FillRandomImpl(size_t startSector, size_t sectorCount, bool useSeed, UInt32 seed = 0);
		void FillSectorsWithRandomData(Random32& random, size_t startSector, size_t sectorCount);
		void FillPattern(size_t patternStart, size_t patternLen, size_t startByte, size_t endByte);
		//inline void ValidateIndex(size_t index) const;

		inline void ValidateIndex(size_t index) const
		{
			if (index >= GetTotalBytes())
			{
				std::stringstream ss;
				ss  << "Index (" << ufs::utils::Hex(index) 
					<< " is greater than or equal to TotalBytes ("
					<< ufs::utils::Hex(GetTotalBytes())	<< ").";
				throw ufs::OutOfRangeError(ss.str());
			}
		}

		void ValidateCounterMax(size_t maxValue);

		void Copy(UInt8* srcData, size_t startByte, size_t bytesToCopy);
		size_t ValidateCopyParameters(const Buffer& otherBuffer, size_t otherStartSector,
													 size_t startSector, size_t sectorCount);

		//compression routines
		int def(FILE *sourceFile, FILE *destFile, int level);
		int inf(FILE *sourceFile, FILE *destFile);
		void zerr(int result);

		void Initialize(size_t sectorCount, size_t bytesPerSector, bool bMakeAvailableToGui=true);

		UInt8* CalculateDataStart(UInt8* data, size_t dataByteCount);
		void CalculateTotalBytesToAllocate(size_t dataByteCount);
		void FillCompressionInfo(UInt8 type, UInt8 pattenLen, size_t startByte, size_t endByte);

	public: //(Con|De)structors
		Buffer();
		explicit Buffer(size_t sectorCount);
		Buffer(size_t sectorCount, size_t bytesPerSector);
		Buffer(size_t sectorCount, size_t bytesPerSector, bool bMakeAvailableToGui);


		Buffer(const Buffer& buffer);
		virtual ~Buffer();

	public: // Static members

		//prototype for python list -> vector conversion
		//can be deleted once brandon figures this pattern out
		//Buffer& FillBytesNew(const std::vector<UInt8>& pList, size_t startSector, size_t sectorCount);

	public: // C++ only methods
		bool IsAllZeros();
		inline size_t GetDataBufferSize() const { return _dataBufferSize; }

	public: //Python exposed properties
		//size_t GetBytesPerSector() const;
		/// <summary>
		/// Returns the number of bytes in each sector in the buffer.
		/// </summary>
		inline size_t GetBytesPerSector() const { return _bytesPerSector; }
		std::string GetName() const;
		void SetName(const std::string& name);

		//size_t GetTotalBytes() const;
		/// <summary>
		/// Returns the total number of bytes in the buffer (SectorCount * BytesPerSector).
		/// </summary>
		inline size_t GetTotalBytes() const { return _sectorCount * _bytesPerSector; }

		//size_t GetSectorCount() const;
		/// <summary>
		/// Returns the number of sectors in the buffer.
		/// </summary>
		inline size_t GetSectorCount() const { return _sectorCount; }


	public: //Python exposed methods

		UInt8 CalculateChecksumByte(size_t startByte, size_t byteCount);

		UInt64 GetBitCount() const;
		UInt64 GetBitCount(size_t startingOffset) const;
		UInt64 GetBitCount(size_t startingOffset, size_t length) const;
		UInt64 GetBitCount(size_t startingOffset, size_t length, UInt8 value) const;

		CompareResult CompareTo(Buffer& buffer);
		CompareResult CompareTo(Buffer& buffer, size_t startSector);
		CompareResult CompareTo(Buffer& buffer, size_t startSector, size_t sectorCount);
		CompareResult CompareTo(Buffer& buffer, size_t startSector, size_t startSector2, size_t sectorCount);

		// Individual byte level access
		UInt8 GetByte(size_t index) const;
		UInt8 GetByteBit(size_t index, UInt8 bit) const;
		Buffer& SetByte(size_t index, UInt8 value);

		// Individual word level access
		UInt16 GetWord(size_t index) const;
		UInt8 GetWordBit(size_t index, UInt8 bit) const;
		Buffer& SetWord(size_t index, UInt16 value);

		UInt16 GetWordBigEndian(size_t index) const;
		UInt8 GetWordBitBigEndian(size_t index, UInt8 bit) const;
		Buffer& SetWordBigEndian(size_t index, UInt16 value);

		// Individual dword level access
		UInt32 GetDWord(size_t index) const;
		UInt8 GetDWordBit(size_t index, UInt8 bit) const;
		Buffer& SetDWord(size_t index, UInt32 value);

		UInt32 GetDWordBigEndian(size_t index) const;
		UInt8 GetDWordBitBigEndian(size_t index, UInt8 bit) const;
		Buffer& SetDWordBigEndian(size_t index, UInt32 value);

		// Individual qword level access
		UInt64 GetQWord(size_t index) const;
		UInt8 GetQWordBit(size_t index, UInt8 bit) const;
		Buffer& SetQWord(size_t index, UInt64 value);

		UInt64 GetQWordBigEndian(size_t index) const;
		UInt8 GetQWordBitBigEndian(size_t index, UInt8 bit) const;
		Buffer& SetQWordBigEndian(size_t index, UInt64 value);



		boost::shared_ptr<std::vector<UInt8>> GetBytes() const;
		boost::shared_ptr<std::vector<UInt8>> GetBytes(size_t startingOffset) const;
		boost::shared_ptr<std::vector<UInt8>> GetBytes(size_t startingOffset, size_t length) const;
		Buffer& SetBytes(size_t startingOffset, std::vector<UInt8> value);

		std::string GetString() const;
		std::string GetString(size_t startingOffset) const;
		std::string GetString(size_t startingOffset, size_t length) const;
		Buffer& SetString(size_t startingOffset, const std::string& value);

		// Gets pointer needed by low level calls
		UInt8* GetDataStart() const;
		UInt8* GetAllocationStart() const;

		// Sector based fill methods
		Buffer& Fill(UInt8 value);
		Buffer& Fill(UInt8 value, size_t startSector);
		Buffer& Fill(UInt8 value, size_t startSector, size_t sectorCount);

		Buffer& FillAddressOverlay();
		Buffer& FillAddressOverlay(UInt64 startingValue);
		Buffer& FillAddressOverlay(UInt64 startingValue, size_t startSector);
		Buffer& FillAddressOverlay(UInt64 startingValue, size_t startSector, size_t sectorCount);

		Buffer& FillDecrementing();
		Buffer& FillDecrementing(UInt8 startingValue);
		Buffer& FillDecrementing(UInt8 startingValue, size_t startSector);
		Buffer& FillDecrementing(UInt8 startingValue, size_t startSector, size_t sectorCount);

		Buffer& FillIncrementing();
		Buffer& FillIncrementing(UInt8 startingValue);
		Buffer& FillIncrementing(UInt8 startingValue, size_t startSector);
		Buffer& FillIncrementing(UInt8 startingValue, size_t startSector, size_t sectorCount);

		Buffer& FillOnes();
		Buffer& FillOnes(size_t startSector);
		Buffer& FillOnes(size_t startSector, size_t sectorCount);

		Buffer& FillRandom();
		Buffer& FillRandom(size_t startSector);
		Buffer& FillRandom(size_t startSector, size_t sectorCount);

		Buffer& FillRandomSeeded(UInt32 seed);
		Buffer& FillRandomSeeded(UInt32 seed, size_t startSector);
		Buffer& FillRandomSeeded(UInt32 seed, size_t startSector, size_t sectorCount);

		Buffer& FillRandomSeededBySector(UInt32 seed);
		Buffer& FillRandomSeededBySector(UInt32 seed, size_t startSector);
		Buffer& FillRandomSeededBySector(UInt32 seed, size_t startSector, size_t sectorCount);


		// PATRLBAFAST???

		Buffer& FillZeros();
		Buffer& FillZeros(size_t startSector);
		Buffer& FillZeros(size_t startSector, size_t sectorCount);

		Buffer& FillBytes(const std::vector<UInt8>& list);
		Buffer& FillBytes(const std::vector<UInt8>& list, size_t startSector);
		Buffer& FillBytes(const std::vector<UInt8>& list, size_t startSector, size_t sectorCount);

		// File IO methods. The Load methods' parameters are bytes, not sectors!!!
		Buffer& LoadFromFileCompressedBinary(const std::string& fileName);
		Buffer& LoadFromFileCompressedBinary(const std::string& fileName, size_t startingOffset);
		Buffer& LoadFromFileCompressedBinary(const std::string& fileName, size_t startingOffset, size_t length);

		Buffer& LoadFromFileBinary(const std::string& fileName);
		Buffer& LoadFromFileBinary(const std::string& fileName, size_t startingOffset);
		Buffer& LoadFromFileBinary(const std::string& fileName, size_t startingOffset, size_t length);

		Buffer& SaveToFileCompressedBinary(const std::string& fileName);
		Buffer& SaveToFileCompressedBinary(const std::string& fileName, size_t startSector);
		Buffer& SaveToFileCompressedBinary(const std::string& fileName, size_t startSector, size_t sectorCount);

		Buffer& SaveToFileBinary(const std::string& fileName);
		Buffer& SaveToFileBinary(const std::string& fileName, size_t startSector);
		Buffer& SaveToFileBinary(const std::string& fileName, size_t startSector, size_t sectorCount);

		Buffer& AppendToFileBinary(const std::string& fileName);
		Buffer& AppendToFileBinary(const std::string& fileName, size_t startSector);
		Buffer& AppendToFileBinary(const std::string& fileName, size_t startSector, size_t sectorCount);

		Buffer& SaveToFileAscii(const std::string& fileName);
		Buffer& SaveToFileAscii(const std::string& fileName, size_t startSector);
		Buffer& SaveToFileAscii(const std::string& fileName, size_t startSector, size_t sectorCount);

		// Clears any existing data
		Buffer& Resize(size_t sectorCount);
		Buffer& Resize(size_t sectorCount, size_t bytesPerSector);

		//string ToString(size_t startSector = 0, size_t sectorCount = 0, ByteGrouping = Byte) const;
		std::string ToString() const;
		std::string ToString(size_t startSector, size_t sectorCount) const;
		std::string ToString(size_t startSector, size_t sectorCount, ufs::ByteGrouping ByteGrouping) const;

		Buffer& CopyTo(Buffer& destinationBuffer);
		Buffer& CopyTo(Buffer& destinationBuffer, size_t startSector);
		Buffer& CopyTo(Buffer& destinationBuffer, size_t startSector, size_t destStartSector);
		Buffer& CopyTo(Buffer& destinationBuffer, size_t startSector, size_t destStartSector, size_t sectorCount);

		Buffer& CopyFrom(const Buffer& sourceBuffer);
		Buffer& CopyFrom(const Buffer& sourceBuffer, size_t startSector);
		Buffer& CopyFrom(const Buffer& sourceBuffer, size_t startSector, size_t srcStartSector);
		Buffer& CopyFrom(const Buffer& sourceBuffer, size_t startSector, size_t srcStartSector, size_t sectorCount);

		size_t GetLastReadSectorCount() const;
	};
}
#endif
