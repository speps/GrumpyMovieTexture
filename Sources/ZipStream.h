#pragma once

#define MINIZ_HEADER_FILE_ONLY
#include <miniz.c>
#include <stdio.h>
#include <stdint.h>
#include <memory>

typedef void (*ZipStreamLogCallback)(void* userData, const char* text);

class ZipStream
{
public:
    ZipStream(void* userData, ZipStreamLogCallback logCallback)
        : _debugEnabled(false)
        , _userData(userData)
        , _logCallback(logCallback)
        , _index(-1)
        , _crc32(MZ_CRC32_INIT)
        , _fileStart(0)
        , _fileOffset(0)
        , _readBufferSize(MZ_ZIP_MAX_IO_BUF_SIZE)
        , _readBuffer(nullptr)
        , _readSize(0)
        , _readOffset(0)
        , _decompressBufferSize(TINFL_LZ_DICT_SIZE)
        , _decompressBuffer(nullptr)
        , _decompressedSize(0)
        , _decompressedOffset(0)
    {}

    bool open(const char* zipname, const char* entryname);
    size_t read(uint8_t* buffer, size_t bufferSize);

    bool isOpen() const
    {
        return _index != -1;
    }
    bool checksum() const
    {
        return _crc32 == _filestat.m_crc32;
    }
    void setDebugEnabled(bool enabled)
    {
        _debugEnabled = enabled;
    }

    void close();
private:
    bool fileChecks();
    size_t readInternal(uint8_t* buffer, size_t bufferSize);
    void log(const char* format, ...);

    bool _debugEnabled;
    void* _userData;
    ZipStreamLogCallback _logCallback;
    mz_zip_archive _zip;
    int _index;
    mz_zip_archive_file_stat _filestat;
    mz_uint64 _fileStart;
    mz_uint64 _fileOffset;
    mz_ulong _crc32;
    tinfl_decompressor _inflator;

    size_t _readBufferSize;
    std::unique_ptr<uint8_t[]> _readBuffer;
    size_t _readSize, _readOffset;

    size_t _decompressBufferSize;
    std::unique_ptr<uint8_t[]> _decompressBuffer;
    size_t _decompressedSize, _decompressedOffset;
};
