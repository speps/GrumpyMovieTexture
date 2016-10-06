#include <miniz.c>
#include "ZipStream.h"
#include <string.h>

bool ZipStream::open(const char* zipname, const char* entryname)
{
    close();

    // Now try to open the archive.
    memset(&_zip, 0, sizeof(_zip));
    _crc32 = MZ_CRC32_INIT;

    mz_bool status = mz_zip_reader_init_file(&_zip, zipname, 0);
    if (!status)
    {
        return false;
    }

    _index = mz_zip_reader_locate_file(&_zip, entryname, "", 0);
    if (_index == -1)
    {
        close();
        return false;
    }

    status = mz_zip_reader_file_stat(&_zip, _index, &_filestat);
    if (!status)
    {
        close();
        return false;
    }

    mz_uint32 local_header_u32[(MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(mz_uint32) - 1) / sizeof(mz_uint32)]; mz_uint8 *pLocal_header = (mz_uint8 *)local_header_u32;

    // Read and parse the local directory entry.
    _fileStart = _filestat.m_local_header_ofs;
    if (_zip.m_pRead(_zip.m_pIO_opaque, _fileStart, pLocal_header, MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
    {
        close();
        return false;
    }
    if (MZ_READ_LE32(pLocal_header) != MZ_ZIP_LOCAL_DIR_HEADER_SIG)
    {
        close();
        return false;
    }

    _fileStart += MZ_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_FILENAME_LEN_OFS) + MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_EXTRA_LEN_OFS);
    if ((_fileStart + _filestat.m_comp_size) > _zip.m_archive_size)
    {
        close();
        return false;
    }

    _fileOffset = 0;

    tinfl_init(&_inflator);

    if (_filestat.m_method != 0)
    {
        _readBuffer = static_cast<uint8_t*>(_zip.m_pAlloc(_zip.m_pAlloc_opaque, 1, _readBufferSize));
        if (_readBuffer == nullptr)
        {
            close();
            return false;
        }
        _decompressBuffer = static_cast<uint8_t*>(_zip.m_pAlloc(_zip.m_pAlloc_opaque, 1, _decompressBufferSize));
        if (_decompressBuffer == nullptr)
        {
            close();
            return false;
        }
    }

    return true;
}

size_t ZipStream::read(uint8_t* buffer, size_t bufferSize)
{
    if (!_filestat.m_method)
    {
        size_t nRead = readInternal(buffer, bufferSize);
        _crc32 = mz_crc32(_crc32, buffer, nRead);
        return nRead;
    }

    size_t nLeftToWrite = bufferSize;
    while (nLeftToWrite > 0)
    {
        if (_decompressedOffset < _decompressedSize)
        {
            size_t nWrote = MZ_MIN(_decompressedSize - _decompressedOffset, nLeftToWrite);
            uint8_t* bufferToWrite = &buffer[bufferSize - nLeftToWrite];
            uint8_t* bufferToRead = &_decompressBuffer[_decompressedOffset];
            memcpy(bufferToWrite, bufferToRead, nWrote);
            _crc32 = mz_crc32(_crc32, bufferToWrite, nWrote);
            _decompressedOffset += nWrote;
            nLeftToWrite -= nWrote;
            continue;
        }

        _decompressedOffset = 0;
        _decompressedSize = 0;
        while (_decompressedSize < _decompressBufferSize)
        {
            if (_readOffset == _readSize)
            {
                _readSize = readInternal(_readBuffer, _readBufferSize);
                _readOffset = 0;
            }
            size_t nRead = _readSize - _readOffset;
            uint8_t* readBuffer = &_readBuffer[_readOffset];
            size_t nWrote = _decompressBufferSize - _decompressedSize;
            uint8_t* writeBuffer = &_decompressBuffer[_decompressedSize];
            tinfl_status status = tinfl_decompress(&_inflator, readBuffer, &nRead, _decompressBuffer, writeBuffer, &nWrote, _readSize < _readBufferSize ? 0 : TINFL_FLAG_HAS_MORE_INPUT);
            _readOffset += nRead;
            _decompressedSize += nWrote;
            if (status == TINFL_STATUS_DONE)
            {
                break;
            }
        }

        if (_decompressedSize == 0)
        {
            break;
        }
    }

    return bufferSize - nLeftToWrite;
}

void ZipStream::close()
{
    mz_zip_reader_end(&_zip);
    _index = -1;
    _fileStart = 0;
    _fileOffset = 0;
    _readBuffer = nullptr;
    _readSize = 0;
    _readOffset = 0;
    _decompressBuffer = nullptr;
    _decompressedSize = 0;
    _decompressedOffset = 0;
}

bool ZipStream::fileChecks()
{
    // Empty file, or a directory (but not always a directory - I've seen odd zips with directories that have compressed data which inflates to 0 bytes)
    if (!_filestat.m_comp_size)
        return true;

    // Entry is a subdirectory (I've seen old zips with dir entries which have compressed deflate data which inflates to 0 bytes, but these entries claim to uncompress to 512 bytes in the headers).
    if (mz_zip_reader_is_file_a_directory(&_zip, _index))
        return false;

    // Encryption and patch files are not supported.
    if (_filestat.m_bit_flag & (1 | 32))
        return false;

    // This function only supports stored and deflate.
    if ((_filestat.m_method != 0) && (_filestat.m_method != MZ_DEFLATED))
        return false;

    return true;
}

size_t ZipStream::readInternal(uint8_t* buffer, size_t bufferSize)
{
    size_t numLeft = MZ_MIN(_filestat.m_comp_size - _fileOffset, bufferSize);
    size_t nRead = _zip.m_pRead(_zip.m_pIO_opaque, _fileStart + _fileOffset, buffer, numLeft);
    _fileOffset += nRead;
    return nRead;
}
