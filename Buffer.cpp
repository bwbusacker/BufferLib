//==============================================================================
//         Copyright 2022 Micron Technology, Inc. All Rights Reserved.
// 
//    THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND TRADE SECRETS OF 
//   MICRON TECHNOLOGY, INC. USE, DISCLOSURE, OR REPRODUCTION IS PROHIBITED 
//   WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF MICRON TECHNOLOGY, INC.
//==============================================================================

//#include "dmx/Precompiled.h"

#include "Buffer.h"
#include "CompareResult.h"
#include "TypeDefs.h"
#include "Utils.h"

#include <climits>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <boost/random.hpp>



// NOTE: We are limiting the OMP parallization to 2 threads because at any more
// than that the CPU usage gets out of control. We probably need to roll our
// own threading, possibly using boost threads, to get any more performance.
// const int NUM_OMP_THREADS = 4; // Unused when OpenMP not available


namespace
{
	std::string ByteArrayToString(UInt8* bytes, size_t startByte, size_t endByte, size_t sectorSize, ufs::ByteGrouping grouping)
	{
		std::stringstream ss;
		ufs::utils::InitHexStringStream(ss);


		for (size_t i = startByte; i<endByte; i++)
		{
			// new block starting. Don't print block numbers if no sector size passed in.
			if ((sectorSize > 0) && (i % sectorSize == 0))
			{
				// New line before all but first block.
				if (i > 0)
				{
					ss << std::endl;
				}

				ss << "Block " << (i / sectorSize);
			}

			size_t sectorOffset = sectorSize > 0 ? i % sectorSize : i;

			// beginning line...byte count
			if (sectorOffset % 16 == 0)
			{
				ss.width(6);
				ss.fill('0');
				ss << std::endl << i << "    ";
			}

			// output each byte
			ss.width(2);
			ss.fill('0');
			ss << (int)bytes[i];

			// insert space between grouping of data
			if ((((sectorOffset + 1) * 2) % (int)grouping) == 0)
			{
				ss << " ";
			}

			// output of asci data at end of line
			if (((sectorOffset + 1) % 16 == 0) || ((sectorOffset + 1) == sectorSize))
			{

				// Handle filling spaces after hex data if buffer size isn't a
				// multiple of 16.
				int filler = 0;
				while ((sectorOffset + 1 + filler) % 16 != 0)
				{
					ss << "  ";
					// insert space between grouping of data
					if ((((sectorOffset + 1 + filler) * 2) % (int)grouping) == 0)
					{
						ss << " ";
					}
					filler++;
				}

				// Add 3 spaces after the last space so we have 4 spaces between
				// the hex values and the ASCI output.
				ss << "   ";

				int charsInThisLine = ((sectorOffset + 1) % 16);
				charsInThisLine = (charsInThisLine == 0) ? 16 : charsInThisLine;
				for (size_t asciiPointer = i - charsInThisLine + 1; asciiPointer <= i; asciiPointer++)
				{
					UInt8 sendIt = bytes[asciiPointer];

					// Replace all control and extended characters with ".".
					// Not all environments and fonts deal with the control
					// or extended characters well.
					if (sendIt < 32 || sendIt > 126)
					{
						sendIt = '.';
					}

					ss << sendIt;
				}
			}
		}

		ss << std::endl;

		return ss.str();
	}
}



/// <summary>
/// Buffer copy constructor
/// </summary>
/// <param name = "buffer">
/// The buffer to copy.
/// </param>
ufs::Buffer::Buffer(const ufs::Buffer& buffer)
{
	this->_bytesPerSector = buffer._bytesPerSector;
	this->_sectorCount = buffer._sectorCount;
	this->_random = 0;
	this->_usePatternMode = buffer._usePatternMode;

	if (buffer._random != 0)
	{
		this->_random = new Random32(*buffer._random);
	}

	//should we avoid copying the name?
	this->_allocatedByteCount = buffer._allocatedByteCount;
	this->_dataBufferSize = buffer._dataBufferSize;

	_data = new UInt8[_allocatedByteCount];

	_dataStart = CalculateDataStart(_data, GetDataBufferSize());

	::memcpy(_dataStart, buffer.GetDataStart(), buffer.GetTotalBytes());

}

/// <summary>
/// Creates a new Buffer object. Equivalent: dmx.Buffer(0x10000, 512).
/// </summary>
ufs::Buffer::Buffer()
	: _allocatedByteCount(0)
{
	Initialize(ufs::DEFAULT_SECTOR_COUNT, ufs::DEFAULT_BYTES_PER_SECTOR);
}

/// <summary>
/// Creates a new Buffer object. Equivalent: dmx.Buffer(sectorCount, 512).
/// </summary>
/// <param name = "sectorCount">
/// The number of sectors in the new buffer.
/// </param>
ufs::Buffer::Buffer(size_t sectorCount)
	: _allocatedByteCount(0)
{
	Initialize(sectorCount, ufs::DEFAULT_BYTES_PER_SECTOR);
}

/// <summary>
/// Creates a new buffer object and fills with zeros.
/// </summary>
/// <param name = "sectorCount">
/// The number of sectors in the new buffer.
/// </param>
/// <param name = "bytesPerSector">
/// The number of bytes in each sector in the new buffer.
/// </param>
ufs::Buffer::Buffer(size_t sectorCount, size_t bytesPerSector)
	: _allocatedByteCount(0)
{
	Initialize(sectorCount, bytesPerSector);
}

// Not exposed to Python. Used internally when creating buffers that should not be exposed to the gui
ufs::Buffer::Buffer(size_t sectorCount, size_t bytesPerSector, bool bMakeAvailableToGui)
	: _allocatedByteCount(0)
{
	Initialize(sectorCount, bytesPerSector, bMakeAvailableToGui);
}

void ufs::Buffer::Initialize(size_t sectorCount, size_t bytesPerSector, bool /*bMakeAvailableToGui*/)
{
	_random = 0;

	// Simulator compression mode will be enabled if DMX_SIMULATOR_ENABLED != 0.
	// Data generation rule info will embed into data buffer.
	// Simulator can recover data by using these rules.
	char *dmxSimulatorEnabled = getenv("DMX_SIMULATOR_ENABLED");
	if (dmxSimulatorEnabled == NULL)
	{
		_usePatternMode = 0;
	}
	else
	{
		_usePatternMode = atoi(dmxSimulatorEnabled);
	}

	if( sectorCount < 1)
	{
		throw ufs::ArgumentError("sectorCount must be greater than zero.");
	}

	if( bytesPerSector < 1)
	{
		throw ufs::ArgumentError("bytesPerSector must be greater than zero.");
	}

	_name = "";

	_bytesPerSector = bytesPerSector;
	_sectorCount = sectorCount;

	CalculateTotalBytesToAllocate(GetTotalBytes());
	_data = new UInt8[_allocatedByteCount];

	_dataStart = CalculateDataStart(_data, GetDataBufferSize());

	// This IS necessary!!
	//FillZeros();
	Fill(0, 0, sectorCount);
}

ufs::Buffer::~Buffer()
{
	delete [] _data;

	if (_random)
	{
		delete _random;
	}
}

void ufs::Buffer::CalculateTotalBytesToAllocate(size_t dataByteCount)
{
	// Allocated bytes are not necessarily on a 4K boundary. Need to allocate enough memory
	// so that the data start can be 4K aligned and another 4K to support putting UFS structures before
	// the start of the data (still used by users' simulator driver).

	size_t dataByteCountLowBits = dataByteCount & 0xFFF;
	size_t alignmentOffset = dataByteCountLowBits == 0 ? 0 : 0x1000 - dataByteCountLowBits;
	_dataBufferSize = dataByteCount + alignmentOffset;
	_allocatedByteCount = _dataBufferSize + 4095 + 4096;
}

UInt8* ufs::Buffer::CalculateDataStart(UInt8* data, size_t dataByteCount)
{
	// Calculate an offset to achieve a 4K buffer alignment
	UInt64 dataStartLowBits = ((UInt64)data) & 0xFFF;
	UInt64 alignmentOffset = dataStartLowBits == 0 ? 0 : 0x1000 - dataStartLowBits;
	UInt8* dataStart = (UInt8*)((UInt64)data + alignmentOffset);

	// Increase the data start by another 4K to support putting UFS structures before
	// the start of the data
	dataStart += 0x1000;

	// Sanity check to make sure all data can fit in the allocated memory.
	if((UInt64)dataStart + (UInt64)dataByteCount > (UInt64)data + (UInt64)_allocatedByteCount)
	{
		throw ufs::RuntimeError("Buffer allocation error");
	}

	return dataStart;
}

//
// Public Properties
//

/// <summary>
/// Gets or sets the Name of the buffer.
/// </summary>
std::string ufs::Buffer::GetName() const
{
	return _name;
}

/// <summary>
/// Sets the Name of the buffer.
/// </summary>
/// <param name="name">String assigned as name of the buffer.</param>
void ufs::Buffer::SetName(const std::string& name)
{
	_name = name;
}


/// <summary>
/// Returns the buffer if is filled all zeros.
/// </summary>
bool ufs::Buffer::IsAllZeros()
{
	if (this->GetDataStart()[0] == 0)
	{
		// compare two buffers : buf[0, size-1] and buf[1, size]
		// 0 to 1, 1 to 2, ..., size-2 to size-1
		// becuase buf[0] is 0, so it works by this way.
		return !::memcmp(this->GetDataStart(), this->GetDataStart() + 1, this->GetTotalBytes() - 1);
	}

	return false;
}

//
// Public Methods
//

/// <summary>
/// Calculates a checksum byte value for the given range of bytes.
///
/// The checksum is the two's complement of the sum of all bytes starting at
/// startByte for byteCount bytes. Each byte is added with unsigned arithmetic
/// and overflow is ignored. The sum of all bytes used to calculate the
/// checksum plus the checksum is zero when the checksum is correct.
///
/// | Typical use is to write a checksum in the last sector of a buffer
/// similar to this:
///
///   ``buf.SetByte(511, buf.CalculateChecksumByte(0, 511))``
///
/// </summary>
/// <param name="startByte">The index of the first byte used to calculate the checksum.</param>
/// <param name="byteCount">The number of bytes uses to calculate the checksum.</param>
UInt8 ufs::Buffer::CalculateChecksumByte(size_t startByte, size_t byteCount)
{
	if(byteCount == 0)
	{
		throw ufs::ArgumentError("byteCount must be greater than 0.");
	}

	if(startByte + byteCount > GetTotalBytes())
	{
		throw ufs::OutOfRangeError("startByte + byteCount is greater that TotalBytes.");
	}

	UInt8 result = 0;

	for(size_t i = startByte; i < (startByte + byteCount); i++)
	{
		// Access pointer directly since we already did our bounds checking.
		result = static_cast<UInt8>(result + _dataStart[i]);
	}

	// 2's complement calculation is the complement of the sum of the values, plus 1;
	result = static_cast<UInt8>((~result) + 1);
	return result;
}

/// <summary>
/// Equivalent:  dmx.Buffer.GetBitCount(0)
/// </summary>
UInt64 ufs::Buffer::GetBitCount() const
{
	return GetBitCount(0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.GetBitCount(startingOffset, 0)
/// </summary>
/// <param name="startingOffset">
/// The starting offset in bytes.  Default is 0.
/// </param>
UInt64 ufs::Buffer::GetBitCount(size_t startingOffset) const
{
	return GetBitCount(startingOffset, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.GetBitCount(startingOffset, length, 1)
/// </summary>
/// <param name="startingOffset">
/// The starting offset in bytes.  Default is 0.
/// </param>
/// <param name="length">
/// The length of bytes to count bits in, or 0 to count bits until the end.  Default is 0.
/// </param>
UInt64 ufs::Buffer::GetBitCount(size_t startingOffset, size_t length) const
{
	return GetBitCount(startingOffset, length, 1);
}

/// <summary>
/// Returns the number of 0 or 1 bits in a range of bytes.
/// </summary>
/// <param name="startingOffset">
/// The starting offset in bytes.  Default is 0.
/// </param>
/// <param name="length">
/// The length of bytes to count bits in, or 0 to count bits until the end.  Default is 0.
/// </param>
/// <param name="value">
/// The bit value, 0 or 1, to count.  Default is 1.
/// </param>
UInt64 ufs::Buffer::GetBitCount(size_t startingOffset, size_t length, UInt8 value) const
{
	size_t newLength = ValidateByteRangeAndGetLength(startingOffset, length);

	UInt8 invertMaskForCountingZeroes = 0x00;
	if (value == 0)
	{
		// To count 0 bits, invert the bits and count the 1 bits.
		invertMaskForCountingZeroes = 0xFF;
	}

	// For performance, use a lookup table of the count of 1s in each byte.
	static UInt64 lookup[256];
	if (lookup[255] == 0)	// Lazy initialization (lookup[255] is 0xFF, should be 8)
	{
		for (int i = 0; i < 256; ++i)
		{
			UInt64 count = 0;
			for (int bit = 0; bit < 8; ++bit)
			{
				count += (i >> bit) & 0x1;
			}
			lookup[i] = count;
		}
	}

	UInt64 bitCount = 0;
	const size_t endByte = startingOffset + newLength;
	for (size_t i = startingOffset; i < endByte; ++i)
	{
		const UInt8 val = _dataStart[i] ^ invertMaskForCountingZeroes;
		bitCount += lookup[val];
	}
	return bitCount;
}

/// <summary>
/// Equivalent:  dmx.Buffer.CompareTo(buffer, 0)
/// </summary>
/// <param name = "buffer">
/// The buffer to compare this buffer object to.
/// </param>
ufs::CompareResult ufs::Buffer::CompareTo(Buffer& buffer)
{
	return CompareTo(buffer, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.CompareTo(buffer, startSector, 0)
/// </summary>
/// <param name = "buffer">
/// The buffer to compare this buffer object to.
/// </param>
/// <param name = "startSector">
/// The sector to start the comparison at.
/// </param>
ufs::CompareResult ufs::Buffer::CompareTo(Buffer& buffer, size_t startSector)
{
	return CompareTo(buffer, startSector, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.CompareTo(buffer, startSector, startSector, sectorCount)
/// </summary>
/// <param name = "buffer">
/// The buffer to compare this buffer object to.
/// </param>
/// <param name = "startSector">
/// The sector to start the comparison at.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to compare.
/// </param>
ufs::CompareResult ufs::Buffer::CompareTo(Buffer& buffer, size_t startSector, size_t sectorCount)
{
	return CompareTo(buffer, startSector, startSector, sectorCount);
}

/// <summary>
/// Compares this :class:`dmx.Buffer` to another :class:`dmx.Buffer`. Comparison starts
/// for this :class:`dmx.Buffer` at the sector specified by startSector and for the passed
/// in :class:`dmx.Buffer` at the sector specified by startSector2. The number of sectors
/// compared is specified by sectorCount. Returns a :class:`dmx.CompareResult` with the
/// details  of the comparison.
/// </summary>
/// <param name = "buffer">
/// The buffer to compare this buffer object to.
/// </param>
/// <param name = "startSector">
/// The sector to start the comparison at for this buffer.
/// </param>
/// <param name = "startSector2">
/// The sector to start the comparison at for the buffer passed in as "buffer".
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to compare.
/// </param>
ufs::CompareResult ufs::Buffer::CompareTo(Buffer& buffer, size_t startSector, size_t startSector2, size_t sectorCount)
{
	size_t startByte = 0;
	size_t endByte = 0;
	GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

	size_t startByte2 = 0;
	size_t endByte2 = 0;
	buffer.GetStartAndStopBytesFromSectors(startSector2, sectorCount, startByte2, endByte2);

	size_t bytesToCompare = endByte - startByte;

	if(sectorCount == 0)
	{
		// If sectorCount is 0/not passed in, read to the end using the shortest
		// of the two buffer lengths from their associated start points.
		bytesToCompare =  std::min(bytesToCompare, endByte2 - startByte2);
	}
	else
	{
		// If number of sectors was passed in, and we don't calculate the same number of bytes
		// to compare for each buffer, something has gone horribly wrong!
		if((endByte2 - startByte2) != bytesToCompare)
		{
			throw ufs::ArgumentError("Unexpected error. Calculated byte counts to compare for the two buffers does not match.");
		}
	}

	UInt8* left  = _dataStart + startByte;
	UInt8* right = buffer.GetDataStart() + startByte2;

	int result = ::memcmp(
		((void*)left),
		((void*)right),
		bytesToCompare);

	size_t offset = 0;
	size_t offset2 = 0;

	if(result != 0)
	{
		for(size_t i = 0; i < bytesToCompare; i++)
		{
			if(left[i] != right[i])
			{
				offset = startByte + i;
				offset2 = startByte2 + i;
				break;
			}
		}
	}

	if (result == 0) {
		return ufs::CompareResult();  // Buffers are equal
	} else {
		UInt8 expectedValue = _dataStart[offset];
		UInt8 actualValue = buffer._dataStart[offset2];
		return ufs::CompareResult(offset, expectedValue, actualValue);
	}
}


/// <summary>
/// Return a single byte from the buffer.
/// </summary>
/// <param name = "index">
/// The zero based index location of the byte to return, in bytes.
/// </param>
UInt8 ufs::Buffer::GetByte(size_t index) const
{
	ValidateIndex(index);
	return _dataStart[index];
}

/// <summary>
/// Return a single bit from one byte in the buffer.
/// </summary>
/// <param name = "index">
/// The zero based index location of the byte to retrieve the bit from, in bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the byte specified in "index".
/// </param>
UInt8 ufs::Buffer::GetByteBit(size_t index, UInt8 bit) const
{
	if (bit > 7)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a byte.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetByte(index) &  (1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a single byte in the buffer.
/// </summary>
/// <param name = "index">
/// The zero based index location of the byte whose value you wish to set, in bytes.
/// </param>
/// <param name = "value">
/// The byte value to write to the buffer at "index" location.
/// </param>
ufs::Buffer& ufs::Buffer::SetByte(size_t index, UInt8 value)
{
	ValidateIndex(index);
	_dataStart[index] = value;
	return *this;
}

/// <summary>
/// Return a single 16 bit (2 byte) word from the buffer. Byte order is
/// little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the word to return, in 8 bit bytes.
/// </param>
UInt16 ufs::Buffer::GetWord(size_t index) const
{
	ValidateIndex(index + 1);
	// Least significant byte is the lower index.
	return static_cast<UInt16>(_dataStart[index] | (_dataStart[index + 1] << 8));
}

/// <summary>
/// Return a single bit from a 16 bit (2 byte) word in the buffer. Byte order
/// is little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the word to retrieve the bit from,
/// in 8 bit bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the word specified in "index".
/// </param>
UInt8 ufs::Buffer::GetWordBit(size_t index, UInt8 bit) const
{
	if (bit > 15)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a word.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetWord(index) &  (1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a 16 bit (2 byte) word in the buffer. Byte order is
/// little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the word whose value you wish to set,
/// in 8 bit bytes.
/// </param>
/// <param name = "value">
/// The 16 bit value to set starting at the word specified in "index" (least significant byte is first).
/// </param>
ufs::Buffer& ufs::Buffer::SetWord(size_t index, UInt16 value)
{
	ValidateIndex(index + 1);
	// Least significant byte is the lower index.
	_dataStart[index] = static_cast<UInt8>(value & 0xFF);
	_dataStart[index + 1] = static_cast<UInt8>(value >> 8);
	return *this;
}





/// <summary>
/// Return a single 16 bit (2 byte) word from the buffer. Byte order is
/// big endian (most significant byte is first), for SCSI data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the word to return, in 8 bit bytes.
/// </param>
UInt16 ufs::Buffer::GetWordBigEndian(size_t index) const
{
	ValidateIndex(index + 1);
	// Most significant byte is the lower index.
	return static_cast<UInt16>((_dataStart[index] << 8) | _dataStart[index + 1]);
}

/// <summary>
/// Return a single bit from a 16 bit word in the buffer. Byte order is
/// big endian (most significant byte is first), for SCSI data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the word to retrieve the bit from,
/// in 8 bit bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the word specified in "index".
/// </param>
UInt8 ufs::Buffer::GetWordBitBigEndian(size_t index, UInt8 bit) const
{
	if (bit > 15)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a word.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetWordBigEndian(index) &  (1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a 16 bit word (two bytes) in the buffer. Byte order is
/// big endian (most significant byte is first), for SCSI data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the word whose value you wish to set,
/// in 8 bit bytes.
/// </param>
/// <param name = "value">
/// The 16 bit value to set starting at the word specified in "index" (most significant byte is first).
/// </param>
ufs::Buffer& ufs::Buffer::SetWordBigEndian(size_t index, UInt16 value)
{
	ValidateIndex(index + 1);
	// Most significant byte is the lower index.
	_dataStart[index] = static_cast<UInt8>(value >> 8);
	_dataStart[index + 1] = static_cast<UInt8>(value & 0xFF);
	return *this;
}





/// <summary>
/// Return a single 32 bit (4 byte) dword from the buffer. Byte order is
/// little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the dword to return, in 8 bit bytes.
/// </param>
UInt32 ufs::Buffer::GetDWord(size_t index) const
{
	ValidateIndex(index + 3);
	// Least significant byte is the lower index.
	return _dataStart[index]
		| (_dataStart[index + 1] << 8)
		| (_dataStart[index + 2] << 16)
		| (_dataStart[index + 3] << 24);
}

/// <summary>
/// Return a single bit from a 32 bit (4 byte) dword in the buffer. Byte order
/// is little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the dword to retrieve the bit from,
/// in 8 bit bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the word specified in "index".
/// </param>
UInt8 ufs::Buffer::GetDWordBit(size_t index, UInt8 bit) const
{
	if (bit > 31)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a d-word.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetDWord(index) &  (1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a 32 bit (4 byte) dword in the buffer. Byte order is
/// little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the dword whose value you wish to set,
/// in 8 bit bytes.
/// </param>
/// <param name = "value">
/// The 32 bit value to set starting at the word specified in "index" (least significant byte is first).
/// </param>
ufs::Buffer& ufs::Buffer::SetDWord(size_t index, UInt32 value)
{
	ValidateIndex(index + 3);
	// Least significant byte is the lower index.
	_dataStart[index] = static_cast<UInt8>(value & 0xFF);
	_dataStart[index + 1] = static_cast<UInt8>(value >> 8);
	_dataStart[index + 2] = static_cast<UInt8>(value >> 16);
	_dataStart[index + 3] = static_cast<UInt8>(value >> 24);
	return *this;
}




/// <summary>
/// Return a single 32 bit (4 byte) dword from the buffer. Byte order is
/// big endian (most significant byte is first), for SCSI data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the dword to return, in 8 bit bytes.
/// </param>
UInt32 ufs::Buffer::GetDWordBigEndian(size_t index) const
{
	ValidateIndex(index + 3);
	// Most significant byte is the lower index.
	return (_dataStart[index] << 24)
		 | (_dataStart[index + 1] << 16)
		 | (_dataStart[index + 2] << 8)
		 |  _dataStart[index + 3];
}

/// <summary>
/// Return a single bit from a 16 bit (4 byte) dword in the buffer. Byte order
/// is big endian (most significant byte is first), for SCSI data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the dword to retrieve the bit from,
/// in 8 bit bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the word specified in "index".
/// </param>
UInt8 ufs::Buffer::GetDWordBitBigEndian(size_t index, UInt8 bit) const
{
	if (bit > 31)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a d-word.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetDWordBigEndian(index) &  (1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a 32 bit dword (four bytes) in the buffer. Byte order is
/// big endian (most significant byte is first), for SCSI data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the dword whose value you wish to set,
/// in 8 bit bytes.
/// </param>
/// <param name = "value">
/// The 32 bit value to set starting at the word specified in "index" (most significant byte is first).
/// </param>
ufs::Buffer& ufs::Buffer::SetDWordBigEndian(size_t index, UInt32 value)
{
	ValidateIndex(index + 3);
	// Most significant byte is the lower index.
	_dataStart[index] = static_cast<UInt8>(value >> 24);
	_dataStart[index + 1] = static_cast<UInt8>(value >> 16);
	_dataStart[index + 2] = static_cast<UInt8>(value >> 8);
	_dataStart[index + 3] = static_cast<UInt8>(value & 0xFF);
	return *this;
}





/// <summary>
/// Return a single 64 bit (8 byte) qword from the buffer. Byte order is
/// little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the qword to return, in 8 bit bytes.
/// </param>
UInt64 ufs::Buffer::GetQWord(size_t index) const
{
	ValidateIndex(index + 7);
	// Least significant byte is the lower index.
	return (UInt64)_dataStart[index]
		| ((UInt64)_dataStart[index + 1] << 8)
		| ((UInt64)_dataStart[index + 2] << 16)
		| ((UInt64)_dataStart[index + 3] << 24)
		| ((UInt64)_dataStart[index + 4] << 32)
		| ((UInt64)_dataStart[index + 5] << 40)
		| ((UInt64)_dataStart[index + 6] << 48)
		| ((UInt64)_dataStart[index + 7] << 56);
}

/// <summary>
/// Return a single bit from a 64 bit (8 byte) qword in the buffer. Byte order
/// is little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the qword to retrieve the bit from,
/// in 8 bit bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the word specified in "index".
/// </param>
UInt8 ufs::Buffer::GetQWordBit(size_t index, UInt8 bit) const
{
	if (bit > 63)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a q-word.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetQWord(index) &  ((UInt64)1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a 64 bit (8 byte) qword in the buffer. Byte order is
/// little endian (least significant byte is first), for SATA data.
/// </summary>
/// <param name = "index">
/// The zero based index location of the qword whose value you wish to set,
/// in 8 bit bytes.
/// </param>
/// <param name = "value">
/// The 64 bit value to set starting at the word specified in "index" (least significant byte is first).
/// </param>
ufs::Buffer& ufs::Buffer::SetQWord(size_t index, UInt64 value)
{
	ValidateIndex(index + 7);
	// Least significant byte is the lower index.
	_dataStart[index] = static_cast<UInt8>(value);
	_dataStart[index + 1] = static_cast<UInt8>(value >> 8);
	_dataStart[index + 2] = static_cast<UInt8>(value >> 16);
	_dataStart[index + 3] = static_cast<UInt8>(value >> 24);
	_dataStart[index + 4] = static_cast<UInt8>(value >> 32);
	_dataStart[index + 5] = static_cast<UInt8>(value >> 40);
	_dataStart[index + 6] = static_cast<UInt8>(value >> 48);
	_dataStart[index + 7] = static_cast<UInt8>(value >> 56);
	return *this;
}




/// <summary>
/// Return a single 64 bit qword from the buffer.
/// </summary>
/// <param name = "index">
/// The zero based index location of the qword to return, in 8 bit bytes.
/// </param>
UInt64 ufs::Buffer::GetQWordBigEndian(size_t index) const
{
	ValidateIndex(index + 7);
	// Most significant byte is the lower index.
	return ((UInt64)_dataStart[index]) << 56
		| ((UInt64)_dataStart[index + 1] << 48)
		| ((UInt64)_dataStart[index + 2] << 40)
		| ((UInt64)_dataStart[index + 3] << 32)
		| ((UInt64)_dataStart[index + 4] << 24)
		| ((UInt64)_dataStart[index + 5] << 16)
		| ((UInt64)_dataStart[index + 6] << 8)
		| (UInt64)_dataStart[index + 7];
}

/// <summary>
/// Return a single bit from a 64 bit qword in the buffer.
/// </summary>
/// <param name = "index">
/// The zero based index location of the qword to retrieve the bit from,
/// in 8 bit bytes.
/// </param>
/// <param name = "bit">
/// The zero based bit number to retrieve from the word specified in "index".
/// </param>
UInt8 ufs::Buffer::GetQWordBitBigEndian(size_t index, UInt8 bit) const
{
	if (bit > 63)
	{
		boost::format errStr("Invalid bit index. %1% is beyond the width of a q-word.");
		errStr % static_cast<UInt32>(bit);
		throw ufs::ArgumentError(errStr.str());
	}

	return (GetQWordBigEndian(index) &  ((UInt64)1 << bit)) > 0;
}

/// <summary>
/// Sets the value of a 64 bit word (eight bytes) in the buffer.
/// </summary>
/// <param name = "index">
/// The zero based index location of the qword whose value you wish to set,
/// in 8 bit bytes.
/// </param>
/// <param name = "value">
/// The 64 bit value to set starting at the word specified in "index"(most significant byte is first).
/// </param>
ufs::Buffer& ufs::Buffer::SetQWordBigEndian(size_t index, UInt64 value)
{
	ValidateIndex(index + 7);
	// Most significant byte is the lower index.
	_dataStart[index] = static_cast<UInt8>(value >> 56);
	_dataStart[index + 1] = static_cast<UInt8>(value >> 48);
	_dataStart[index + 2] = static_cast<UInt8>(value >> 40);
	_dataStart[index + 3] = static_cast<UInt8>(value >> 32);
	_dataStart[index + 4] = static_cast<UInt8>(value >> 24);
	_dataStart[index + 5] = static_cast<UInt8>(value >> 16);
	_dataStart[index + 6] = static_cast<UInt8>(value >> 8);
	_dataStart[index + 7] = static_cast<UInt8>(value);
	return *this;
}

/// <summary> Gets the data start of the buffer. </summary>
UInt8* ufs::Buffer::GetDataStart() const
{
	return _dataStart;
}

/// <summary> Gets the start of the block of memeory allocated for the buffer. </summary>
UInt8* ufs::Buffer::GetAllocationStart() const
{
	return _data;
}


/// <summary>
///  Equivalent:  dmx.Buffer.GetBytes(0)
/// </summary>
boost::shared_ptr<std::vector<UInt8>> ufs::Buffer::GetBytes() const
{
	return GetBytes(0);
}

/// <summary>
///  Equivalent:  ufs::Buffer::GetBytes(startingOffset, 0)
/// </summary>
///
/// <param name="startingOffset"> The starting offset in bytes. </param>
boost::shared_ptr<std::vector<UInt8>> ufs::Buffer::GetBytes(size_t startingOffset) const
{
	return GetBytes(startingOffset, 0);
}

/// <summary>
///  Return number of bytes from the buffer starting at offset for number of bytes requested.
///  If length == 0 returns buffers total bytes minus startingOffset.
/// </summary>
///
/// <param name="startingOffset"> The starting offset in bytes. </param>
/// <param name="length"> The length. </param>
boost::shared_ptr<std::vector<UInt8>> ufs::Buffer::GetBytes(size_t startingOffset, size_t length) const
{
	size_t newLength = ValidateByteRangeAndGetLength(startingOffset, length);

	boost::shared_ptr<std::vector<UInt8>> ptr(new std::vector<UInt8>(newLength));
	std::copy(_dataStart + startingOffset, _dataStart + startingOffset + newLength, ptr->begin());
	return ptr;
}

/// <summary>
/// Add info related with compression into buffer. e.g. compression type, pattern length.
/// Save pattern in the fixed position
/// compression format:
///    Lba        |   Pattern     | Type(4 higher bits) and Pattern Length(4 lower bits)
/// 8 bytes       |   12 bytes    |          1 byte
/// confluence page: https://confluence.micron.com/confluence/display/FE/Simulator+compression+mode
/// </summary>
void ufs::Buffer::FillCompressionInfo(UInt8 type, UInt8 pattenLen, size_t startByte, size_t endByte)
{
	if (pattenLen > COMPRESSION_MAX_PATTERN_LEN)
	{
		return;
	}

	for (size_t i = startByte + COMPRESSION_LBA_SIZE_IN_BYTE + COMPRESSION_PATTERN_SIZE_IN_BYTE;
		i<endByte;
		i += _bytesPerSector)
	{
		// FTE scripts use dmx to generate a "0" buffer to verify empty page and compressed pattern is not suitable in this case.
		if (!(_dataStart[startByte] == 0 && type == eFixPattern))
		{
			// Add pattern type and patten length
			_dataStart[i] = 0;
			_dataStart[i] = static_cast<UInt8>((type << 4) | pattenLen);
		}
		switch (type)
		{
		case eFixPattern:
		case eIncrementingPattern:
		case eDecrementingPattern:
		{
			// copy pattern to the position beginning with 9th byte
			memcpy(_dataStart + i - COMPRESSION_PATTERN_SIZE_IN_BYTE,
				_dataStart + i - COMPRESSION_PATTERN_SIZE_IN_BYTE - COMPRESSION_LBA_SIZE_IN_BYTE,
				pattenLen);
			break;
		}
		case eRandomPattern:
		{
			break;
		}
		default:
			break;
		}
	}
}

/// <summary>
/// Copies the contents of the list of bytes to the buffer, starting at
/// the specified index.
/// </summary>
///
/// <param name="startingOffset"> The starting offset in bytes. </param>
/// <param name="value"> The value to copy. </param>
ufs::Buffer& ufs::Buffer::SetBytes(size_t startingOffset, std::vector<UInt8> value)
{
	ValidateByteRangeAndGetLength(startingOffset, value.size());
	std::copy(value.begin(), value.end(), _dataStart + startingOffset);
	return *this;
}

/// <summary>
/// Equivalent:  dmx.Buffer.GetString(0)
/// </summary>
std::string ufs::Buffer::GetString() const
{
	return GetString(0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.GetString(startingOffset, 0)
/// </summary>
/// <param name="startingOffset"> The starting offset in bytes. </param>
std::string ufs::Buffer::GetString(size_t startingOffset) const
{
	return GetString(startingOffset, 0);
}

/// <summary>
/// Returns string value of buffer content starting at requested offset
/// for requested length.  If length == 0 returns buffers total bytes minus
/// startingOffset.
/// </summary>
///
/// <param name="startingOffset"> The starting offset in bytes. </param>
/// <param name="length">The length of the string, in bytes. </param>
std::string ufs::Buffer::GetString(size_t startingOffset, size_t length) const
{
	// Validate once up front to avoid validating each byte access.
	size_t newLength = ValidateByteRangeAndGetLength(startingOffset, length);

	return std::string( (char*)(_dataStart + startingOffset), newLength);
}

/// <summary>
/// Copies the contents of the string value to the buffer, starting at
/// the requested starting offset.
/// </summary>
///
/// <param name="startingOffset"> The starting offset in bytes. </param>
/// <param name="value"> The value to copy. </param>
ufs::Buffer& ufs::Buffer::SetString(size_t startingOffset, const std::string& value)
{
	ValidateByteRangeAndGetLength(startingOffset, value.length());
	::memcpy(_dataStart + startingOffset, value.c_str(), value.length());
	return *this;
}


/// <summary>
/// Equivalent:  dmx.Buffer.Fill(value, 0)
/// </summary>
/// <param name = "value">
/// The byte value to set every byte in the buffer to.
/// </param>
ufs::Buffer& ufs::Buffer::Fill(UInt8 value)
{
	return Fill(value, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.Fill(value, startSector, 0)
/// </summary>
/// <param name = "value">
/// The byte value to set the bytes in the buffer to.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::Fill(UInt8 value, size_t startSector)
{
	return Fill(value, startSector, 0);
}


/// <summary>
/// Sets the bytes in the buffer to the value passed in,
/// starting at the sector specified by "startSector",
/// for the number of sectors specified by "sectorCount".
/// </summary>
/// <param name = "value">
/// The byte value to set the bytes in the buffer to.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with "value".
/// </param>
ufs::Buffer& ufs::Buffer::Fill(UInt8 value, size_t startSector, size_t sectorCount)
{
	size_t startByte = 0;
	size_t endByte = 0;
	GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

	::memset(_dataStart + startByte, value, endByte - startByte);
	if (_usePatternMode)
	{
		FillCompressionInfo(eFixPattern, 1, startByte, endByte);
	}
	return *this;
}



/// <summary>
/// Equivalent:  dmx.Buffer.FillOnes(0).
/// </summary>

ufs::Buffer& ufs::Buffer::FillOnes()
{
	return FillOnes(0);
}


/// <summary>
/// Equivalent:  dmx.Buffer.FillOnes(startSector, 0).
/// </summary>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillOnes(size_t startSector)
{
	return FillOnes(startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to 0xFF (every bit set to 1),
/// starting at the sector specified by "startSector", for the number
/// of sectors specified in "sectorCount".
/// </summary>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with 0xFF.
/// </param>
ufs::Buffer& ufs::Buffer::FillOnes(size_t startSector, size_t sectorCount)
{
	return Fill(0xFF, startSector, sectorCount);
}





/// <summary>
/// Equivalent:  dmx.Buffer.FillAddressOverlay(0)
/// </summary>
ufs::Buffer& ufs::Buffer::FillAddressOverlay()
{
	return FillAddressOverlay(0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillAddressOverlay(startingValue, 0)
/// </summary>
/// <param name="startingValue">
/// The value to be used as the address for the first sector.
/// </param>
ufs::Buffer& ufs::Buffer::FillAddressOverlay(UInt64 startingValue)
{
	return FillAddressOverlay(startingValue, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillAddressOverlay(startingValue, startSector, 0)
/// </summary>
/// <param name="startingValue">
/// The value to be used as the address for the first sector.
/// </param>
/// <param name="startSector">
/// The sector to start applying the address overlay at
/// </param>
ufs::Buffer& ufs::Buffer::FillAddressOverlay(UInt64 startingValue, size_t startSector)
{
	return FillAddressOverlay(startingValue, startSector, 0);
}

/// <summary>
/// Sets the first and last eight bytes (64 bits) of "sectorCount" sectors to an
/// address value, starting at the sector specified by "startSector", with the
/// value specified by "startingValue", and incrementing by one for each sector.
/// </summary>
/// <param name="startingValue">
/// The value to be used as the address for the first sector.
/// </param>
/// <param name="startSector">
/// The sector to start applying the address overlay at.
/// </param>
/// <param name="sectorCount">
/// The number of sectors to apply the address overlay to.
/// </param>
ufs::Buffer& ufs::Buffer::FillAddressOverlay(UInt64 startingValue, size_t startSector, size_t sectorCount)
{
	sectorCount = ValidateSectorRangeAndGetSectorCount(startSector, sectorCount);

	// Get number of 8 byte (64 bit) chucks per sector.
	size_t jump = _bytesPerSector/8;

	// 64 bit pointer to the data byte array. Indexing into
	// this moves us 8 bytes at a time instead of 1.
	UInt64* data64BitPtr = (UInt64*)_dataStart;

	size_t start64bitOffset = startSector * jump;
	size_t end64bitOffset = (startSector + sectorCount) * jump;

	// HACK: In order to use OMP parallelization, we can't use an unsigned counter variable.
	ValidateCounterMax(end64bitOffset);

	bool runLoopInParallel = false; // Unused for now
	if ((startSector < 100 && sectorCount >= 1000) || (startSector >= 100 && sectorCount >= 500))
	{
		runLoopInParallel = true;
	}

	#pragma omp parallel for if(runLoopInParallel) num_threads(NUM_OMP_THREADS)
	for(Int64 i = (Int64)start64bitOffset; i < (Int64)end64bitOffset; i += jump)
	{
		// Calculate the value using existing variables instead of a ++
		// so the OMP parallelization works better.
		UInt64 value = startingValue + (i - start64bitOffset)/jump;
		data64BitPtr[i] = value;
		data64BitPtr[i + jump -1] = value;
	}

	return *this;
}




/// <summary>
/// Equivalent:  dmx.Buffer.FillZeros(0)
/// </summary>
ufs::Buffer& ufs::Buffer::FillZeros()
{
	return FillZeros(0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillZeros(startSector, 0)
/// </summary>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillZeros(size_t startSector)
{
	return FillZeros(startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to 0x0 (every bit set to 0), starting at the
/// sector specified by "startSector", for the number of sectors specified in
/// "sectorCount".
/// </summary>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with 0x0.
/// </param>
ufs::Buffer& ufs::Buffer::FillZeros(size_t startSector, size_t sectorCount)
{
	return Fill(0, startSector, sectorCount);
}



/// <summary>
/// Equivalent:  dmx.Buffer.FillBytes(list, 0).
/// </summary>
/// <param name = "list">
/// The list of bytes to use as a pattern to fill the buffer with.
/// </param>
ufs::Buffer& ufs::Buffer::FillBytes(const std::vector<UInt8>& list)
{
	return FillBytes(list, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillBytes(list, startSector, 0).
/// </summary>
/// <param name = "list">
/// The list of bytes to use as a pattern to fill the buffer with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillBytes(const std::vector<UInt8>& list, size_t startSector)
{
	return FillBytes(list, startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to the pattern specified in "list",
/// repeating the pattern to fill the buffer, starting at the sector specified
/// by "startSector", for the number of sectors specified in "sectorCount".
/// </summary>
/// <param name = "list">
/// The list of bytes to use as a pattern to fill the buffer with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with the pattern.
/// </param>
ufs::Buffer& ufs::Buffer::FillBytes(const std::vector<UInt8>& list, size_t startSector, size_t sectorCount)
{
	size_t byteCount = list.size();

	if (byteCount != 0)
	{
		size_t startByte = 0;
		size_t endByte = 0;
		GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

		size_t patternEnd = startByte + byteCount;

		for (size_t i = startByte; i < std::min(patternEnd, endByte); i++)
		{
			GetDataStart()[i] = list[(i - startByte) % byteCount];
		}

		if (endByte > patternEnd)
		{
			FillPattern(startByte, byteCount, patternEnd, endByte);
		}

		if (_usePatternMode)
		{
			FillCompressionInfo(eFixPattern, static_cast<uint8_t>(byteCount), startByte, endByte);
		}

	}

	return *this;
}




/// <summary>
/// Equivalent:  dmx.Buffer.FillIncrementing(0)
/// </summary>
ufs::Buffer& ufs::Buffer::FillIncrementing()
{
	return FillIncrementing(0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillIncrementing(startValue, 0)
/// </summary>
/// <param name="startingValue">
/// The byte value to start the pattern with.
/// </param>
ufs::Buffer&ufs:: Buffer::FillIncrementing(UInt8 startingValue)
{
	return FillIncrementing(startingValue, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillIncrementing(startValue, startSector, 0)
/// </summary>
/// <param name="startingValue">
/// The byte value to start the pattern with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillIncrementing(UInt8 startingValue, size_t startSector)
{
	return FillIncrementing(startingValue, startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to a sector size boundary, incrementing pattern
/// (0 - 255), starting with "startingValue", starting at the sector specified
/// by "startSector", for the number of sectors specified in "sectorCount".
/// </summary>
/// <param name="startingValue">
/// The byte value to start the pattern with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with the pattern.
/// </param>
ufs::Buffer& ufs::Buffer::FillIncrementing(UInt8 startingValue, size_t startSector, size_t sectorCount)
{
	size_t startByte = 0;
	size_t endByte = 0;
	GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

	for (size_t i = startByte; i < startByte + GetBytesPerSector(); i++)
	{
		_dataStart[i] = startingValue++;
	}

	if (endByte > startByte + GetBytesPerSector())
	{
		FillPattern(startByte, GetBytesPerSector(), startByte + GetBytesPerSector(), endByte);
	}

	if (_usePatternMode)
	{
		FillCompressionInfo(eIncrementingPattern, 1, startByte, endByte);
	}

	return *this;
}




/// <summary>
/// Equivalent:  dmx.Buffer.FillDecrementing(255)
/// </summary>
ufs::Buffer& ufs::Buffer::FillDecrementing()
{
	return FillDecrementing(255);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillDecrementing(startingValue, 0)
/// </summary>
/// <param name="startingValue">
/// The byte value to start the pattern with.
/// </param>
ufs::Buffer& ufs::Buffer::FillDecrementing(UInt8 startingValue)
{
	return FillDecrementing(startingValue, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillDecrementing(startingValue, startSector, 0)
/// </summary>
/// <param name="startingValue">
/// The byte value to start the pattern with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillDecrementing(UInt8 startingValue, size_t startSector)
{
	return FillDecrementing(startingValue, startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to a sector size boundary, decrementing pattern
/// (255 - 0), starting with "startingValue", starting at the sector specified
/// by "startSector", for the number of sectors specified in "sectorCount".
/// </summary>
/// <param name="startingValue">
/// The byte value to start the pattern with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with the pattern.  If sectorCount is zero,
/// fill all sectors.
/// </param>
ufs::Buffer& ufs::Buffer::FillDecrementing(UInt8 startingValue, size_t startSector, size_t sectorCount)
{
	size_t startByte = 0;
	size_t endByte = 0;
	GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

	for (size_t i = startByte; i < startByte + GetBytesPerSector(); i++)
	{
		_dataStart[i] = startingValue--;
	}

	if (endByte > startByte + GetBytesPerSector())
	{
		FillPattern(startByte, GetBytesPerSector(), startByte + GetBytesPerSector(), endByte);
	}

	if (_usePatternMode)
	{
		FillCompressionInfo(eDecrementingPattern, 1, startByte, endByte);
	}

	return *this;
}




/// <summary>
///  Equivalent:  dmx.Buffer.FillRandom(0)
/// </summary>
ufs::Buffer& ufs::Buffer::FillRandom()
{
	return ufs::Buffer::FillRandom(0);
}

/// <summary>
///  Equivalent:  dmx.Buffer.FillRandom(startSector, 0)
/// </summary>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer:: FillRandom(size_t startSector)
{
	return ufs::Buffer::FillRandom(startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to a pseudo-random sequence of values between
/// 0 - 255, starting at the sector specified by "startSector", for the number
/// of sectors specified in "sectorCount". Sequence is  seeded by time and
/// thread id.
/// </summary>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with the pattern.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandom(size_t startSector, size_t sectorCount)
{
	return ufs::Buffer::FillRandomImpl(startSector, sectorCount, false);
}



/// <summary>
/// Equivalent:  dmx.Buffer.FillRandomSeeded(seed, 0)
/// </summary>
/// <param name = "seed">
/// The value to seed the random number generation with.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandomSeeded(UInt32 seed)
{
	return ufs::Buffer::FillRandomSeeded(seed, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillRandomSeeded(seed, startSector, 0)
/// </summary>
/// <param name = "seed">
/// The value to seed the random number generation with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandomSeeded(UInt32 seed, size_t startSector)
{
	return ufs::Buffer::FillRandomSeeded(seed, startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to a pseudo-random sequence of values between
/// 0 - 255, starting at the sector specified by "startSector", for the number
/// of sectors specified in "sectorCount". Sequence is  seeded by the value
/// specified in "seed". Calling this method twice with the same "seed" value
/// will produce the same pseudo-random sequence of values.
/// </summary>
/// <param name = "seed">
/// The value to seed the random number generation with.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill with the pattern.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandomSeeded(UInt32 seed, size_t startSector, size_t sectorCount)
{
	return ufs::Buffer::FillRandomImpl(startSector, sectorCount, true, seed);
}




/// <summary>
/// Equivalent:  dmx.Buffer.FillRandomSeededBySector(seed, 0)
/// </summary>
/// <param name = "seed">
/// The value to seed the random number generation with for the first sector.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandomSeededBySector(UInt32 seed)
{
	return ufs::Buffer::FillRandomSeededBySector(seed, 0);
}

/// <summary>
/// Equivalent:  dmx.Buffer.FillRandomSeededBySector(seed, startSector, 0)
/// </summary>
/// <param name = "seed">
/// The value to seed the random number generation with for the first sector.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandomSeededBySector(UInt32 seed, size_t startSector)
{
	return ufs::Buffer::FillRandomSeededBySector(seed, startSector, 0);
}

/// <summary>
/// Sets the bytes in the buffer to a set of pseudo-random sequences of values
/// between 0 - 255. Each sector is filled with a different sequence by seeding
/// a random number generator with a different number for each sector. The
/// first sector is seeded by the value specified in "seed". The seed value is
/// incremented by one for each subsequent sector. Calling this method twice
/// with the same "seed" value will produce the same set of pseudo-random
/// sequences of values.
/// </summary>
/// <param name = "seed">
/// The value to seed the random number generation with for the first sector.
/// </param>
/// <param name = "startSector">
/// The sector to start filling from.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to fill.
/// </param>
ufs::Buffer& ufs::Buffer::FillRandomSeededBySector(UInt32 seed, size_t startSector, size_t sectorCount)
{
	if(GetBytesPerSector() % 4 != 0)
	{
		throw ufs::RuntimeError("Filling random data is not supported for sector sizes that are not a mupltiple of 4.");
	}

	sectorCount = ValidateSectorRangeAndGetSectorCount(startSector, sectorCount);

	// HACK: In order to use OMP parallelization, we can't use an unsigned counter variable.
	ValidateCounterMax(startSector + sectorCount);

	// This cannot be a reference and be used in an OMP loop.
	ufs::Random32 random = *GetRandom(true, seed);

	// HACK: In order to use OMP parallelization, we can't use an unsigned counter variable.
	ValidateCounterMax(startSector + sectorCount);

	#pragma omp parallel for private (random) num_threads(NUM_OMP_THREADS)
	for(Int64 i = (Int64)startSector; i < (Int64)(startSector + sectorCount); i++)
	{
		random.Seed((UInt32)(seed + i - startSector));
		ufs::Buffer::FillSectorsWithRandomData(random, (size_t)i, 1);
	}

	return *this;
}

ufs::Buffer& ufs::Buffer::FillRandomImpl(size_t startSector, size_t sectorCount, bool useSeed, UInt32 seed)
{
	if(GetBytesPerSector() % 4 != 0)
	{
		throw ufs::RuntimeError("Filling random data is not supported for sector sizes that are not a multiple of 4.");
	}

	ufs::Random32& r = *(GetRandom(useSeed, seed));
	FillSectorsWithRandomData(r, startSector, sectorCount);
	return *this;
}

void ufs::Buffer::FillSectorsWithRandomData(ufs::Random32& random, size_t startSector, size_t sectorCount)
{
	size_t startByte = 0;
	size_t endByte = 0;
	GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

	UInt8 genBuffer[COMPRESSION_PATTERN_SIZE_IN_BYTE];
	size_t sectorByteIndex;

	UInt32 * int32Ptr = (UInt32*)_dataStart;

	// OMP loop won't work here.
	for(size_t i = startByte/4; i < endByte/4; i++)
	{
		if (_usePatternMode)
		{
			/// Embed random data generator(size==12bytes) into buffer from 9th byte to 20th byte
			/// in every sector. With the generator, we can recover every random data.
			/// confluence page: https://confluence.micron.com/confluence/display/FE/Simulator+compression+mode
			sectorByteIndex = i % (_bytesPerSector / 4);
			if (sectorByteIndex == 0)
			{
						// Save the generator state (only copy what fits in buffer)
		const size_t copySize = std::min(sizeof(boost::random::taus88), static_cast<size_t>(COMPRESSION_PATTERN_SIZE_IN_BYTE));
		memcpy(genBuffer, &random.GetGen(), copySize);
			}
			else if (sectorByteIndex == 6)
			{
						// Overlap generator state in sector
		const size_t copySize = std::min(static_cast<size_t>(COMPRESSION_PATTERN_SIZE_IN_BYTE), sizeof(UInt32) * 4);
		memcpy(&int32Ptr[i - 4], genBuffer, copySize);
			}
		}

		int32Ptr[i] = random.Next();
	}

	if (_usePatternMode)
	{
		FillCompressionInfo(eRandomPattern, 0, startByte, endByte);
	}
}



/// <summary>
/// Equivalent:   dmx.Buffer.ToString(0, ufs::Buffer::GetSectorCount())
/// when dmx.Buffer.GetSectorCount() less than 2, otherwise
/// Equivalent:   dmx.Buffer.ToString(0, 2)
/// </summary>
std::string ufs::Buffer::ToString() const
{
	size_t secCount = _sectorCount < 2 ? _sectorCount : 2;

	// Add a vertical elipsis if there are more than 2 sectors.
	if( _sectorCount > 2 )
	{
		return ToString(0, secCount).append(".\n.\n.");
	}
	else
	{
		return ToString(0, secCount);
	}

}

/// <summary>
/// Equivalent:  dmx.Buffer.ToString(0, 256, Byte)
/// </summary>
/// <param name="startSector">
/// The starting sector for formatting.  Default is zero.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to format.  If sectorCount is zero, returns
/// buffer sectorCount or 256 sectors, whichever is less.
/// </param>
std::string ufs::Buffer::ToString(size_t startSector, size_t sectorCount) const
{
	return ToString(startSector, sectorCount, Byte);
}

/// <summary>
/// Outputs buffer data in a formatted layout.
/// For each sector display Block number.
/// For each row of 16 bytes of data, displays starting byte count
/// followed by 16 bytes of data in format specified by Bytegrouping
/// followed by ascii byte values.
/// </summary>
/// <param name="startSector">
/// The starting sector for formatting.  Default is zero.
/// </param>
/// <param name = "sectorCount">
/// The number of sectors to format.  If sectorCount is zero, returns
/// buffer sectorCount or 256 sectors, whichever is less.
/// </param>
/// <param name = "ByteGrouping">
/// Displayed in Byte(2), Word(4), or Dword(8) with space between
/// the groupings.  Default is Byte.
/// </param>
std::string ufs::Buffer::ToString(size_t startSector, size_t sectorCount, ufs::ByteGrouping ByteGrouping) const
{
	std::stringstream ss;
	ufs::utils::InitHexStringStream(ss);

	size_t startByte = 0;
	size_t endByte = 0;
	GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);

	return ::ByteArrayToString(this->_dataStart, startByte, endByte, this->GetBytesPerSector(), ByteGrouping);
}



void ufs::Buffer::FillPattern(size_t patterStart, size_t patternLen, size_t startByte, size_t endByte)
{
	// Loop, doubling the data we copy each time up to 4096 bytes at a time.
	// Big memcpy's are actually slower.
	while(startByte < endByte)
	{
		size_t lenToEndByte = endByte - startByte;
		// Make sure we don't copy past the end byte.
		size_t copyLen = std::min(patternLen, lenToEndByte);
		::memcpy(_dataStart + startByte, _dataStart + patterStart, copyLen);
		startByte += copyLen;
		patternLen = patternLen >= 4096 ? patternLen : patternLen * 2;
	}
}

void ufs::Buffer::ValidateCounterMax(size_t maxValue)
{
	// HACK: In order to use OMP parallelization, we can't use an unsigned counter variable.
	if(maxValue > LLONG_MAX)
	{
		throw ufs::OutOfRangeError("Cannot handle 8 Exabytes of data because OMP loop counter must be signed");
	}
}

ufs::Buffer& ufs::Buffer::CopyTo(Buffer& destinationBuffer) {
    return CopyTo(destinationBuffer, 0, 0, 0);
}

ufs::Buffer& ufs::Buffer::CopyTo(Buffer& destinationBuffer, size_t startSector) {
    return CopyTo(destinationBuffer, startSector, 0, 0);
}

ufs::Buffer& ufs::Buffer::CopyTo(Buffer& destinationBuffer, size_t startSector, size_t destStartSector) {
    return CopyTo(destinationBuffer, startSector, destStartSector, 0);
}

ufs::Buffer& ufs::Buffer::CopyTo(Buffer& destinationBuffer, size_t startSector, size_t destStartSector, size_t sectorCount) {
    size_t startByte = 0, endByte = 0;
    GetStartAndStopBytesFromSectors(startSector, sectorCount, startByte, endByte);
    size_t destStartByte = destStartSector * GetBytesPerSector();
    size_t bytesToCopy = endByte - startByte;
    std::copy(_dataStart + startByte, _dataStart + startByte + bytesToCopy, destinationBuffer._dataStart + destStartByte);
    return destinationBuffer;
}

ufs::Buffer& ufs::Buffer::CopyFrom(const Buffer& sourceBuffer) {
    return CopyFrom(sourceBuffer, 0, 0, 0);
}

ufs::Buffer& ufs::Buffer::CopyFrom(const Buffer& sourceBuffer, size_t startSector) {
    return CopyFrom(sourceBuffer, startSector, 0, 0);
}

ufs::Buffer& ufs::Buffer::CopyFrom(const Buffer& sourceBuffer, size_t startSector, size_t srcStartSector) {
    return CopyFrom(sourceBuffer, startSector, srcStartSector, 0);
}

ufs::Buffer& ufs::Buffer::CopyFrom(const Buffer& sourceBuffer, size_t startSector, size_t srcStartSector, size_t sectorCount) {
    size_t srcStartByte = 0, srcEndByte = 0;
    sourceBuffer.GetStartAndStopBytesFromSectors(srcStartSector, sectorCount, srcStartByte, srcEndByte);
    size_t destStartByte = startSector * GetBytesPerSector();
    size_t bytesToCopy = srcEndByte - srcStartByte;
    std::copy(sourceBuffer._dataStart + srcStartByte, sourceBuffer._dataStart + srcStartByte + bytesToCopy, _dataStart + destStartByte);
    return *this;
}

ufs::Buffer& ufs::Buffer::Resize(size_t sectorCount) {
    return Resize(sectorCount, GetBytesPerSector());
}

ufs::Buffer& ufs::Buffer::Resize(size_t sectorCount, size_t bytesPerSector) {
    // Save current data if we're expanding
    size_t oldTotalBytes = GetTotalBytes();
    size_t newTotalBytes = sectorCount * bytesPerSector;
    size_t bytesToPreserve = std::min(oldTotalBytes, newTotalBytes);
    
    std::vector<UInt8> oldData;
    if (bytesToPreserve > 0) {
        oldData.resize(bytesToPreserve);
        std::copy(_dataStart, _dataStart + bytesToPreserve, oldData.begin());
    }
    
    // Clean up old memory
    delete [] _data;
    if (_random) {
        delete _random;
        _random = 0;
    }
    
    // Initialize with new size
    Initialize(sectorCount, bytesPerSector);
    
    // Restore data if we had any
    if (bytesToPreserve > 0) {
        std::copy(oldData.begin(), oldData.end(), _dataStart);
    }
    
    return *this;
}
