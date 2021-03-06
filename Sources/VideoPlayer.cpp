#include "VideoPlayer.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <regex>

#if __ANDROID__
#include <android/log.h>
void VideoPlayer::log(const char* format, ...) const
{
    if (!_debugEnabled)
    {
        return;
    }
    va_list args;
    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, "LOG_GMT", format, args);
    va_end(args);
}
#elif WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
void VideoPlayer::log(const char* format, ...) const
{
    if (!_debugEnabled)
    {
        return;
    }
    char temp[1024];
    va_list args;
    va_start(args, format);
    vsprintf_s(temp, format, args);
    va_end(args);
    OutputDebugStringA(temp);
    OutputDebugStringA("\n");
}
#else
void VideoPlayer::log(const char* format, ...) const
{
}
#endif

void VideoPlayer::zipLogCallback(void* userData, const char* text)
{
    ((VideoPlayer*)userData)->log(text);
}

VideoPlayer::VideoPlayer(void* userData, VideoStatusCallback statusCallback, VideoLogCallback logCallback, VideoGetValueCallback getValueCallback)
    : _userData(userData), _debugEnabled(false), _state(VideoPlayerState::Initialized), _fileStream(nullptr), _zipStream(this, &zipLogCallback)
    , _timer(0.0), _timerStart(0.0), _timeLastFrame(0.0), _audioBufferSize(VIDEO_PLAYER_AUDIO_BUFFER_SIZE)
    , _audioTotalSamples(0), _videoTime(0.0), _statusCallback(statusCallback), _logCallback(logCallback), _getValueCallback(getValueCallback)
    , _dataCallback(nullptr), _createTextureCallback(nullptr) , _uploadTextureCallback(nullptr), _processVideo(false)
{
}

VideoPlayer::~VideoPlayer()
{
    destroy();
}

void VideoPlayer::destroy()
{
    stop();
    close();
    _statusCallback = nullptr;
    _dataCallback = nullptr;
    _createTextureCallback = nullptr;
    _uploadTextureCallback = nullptr;
}

void VideoPlayer::setState(VideoPlayerState newState)
{
    _state = newState;
    if (_statusCallback != nullptr)
    {
        _statusCallback(_userData, newState);
    }
}

bool VideoPlayer::readStream()
{
    uint8_t* buffer = (uint8_t*)ogg_sync_buffer(&_oggState.oggSyncState, VIDEO_PLAYER_OGG_BUFFER_SIZE);
    if (_dataCallback != nullptr)
    {
        uint32_t bytesRead = 0;
        bool success = _dataCallback(_userData, buffer, VIDEO_PLAYER_OGG_BUFFER_SIZE, &bytesRead);
        ogg_sync_wrote(&_oggState.oggSyncState, bytesRead);
        return success;
    }
    else if (_zipStream.isOpen())
    {
        std::size_t bytesRead = _zipStream.read(buffer, VIDEO_PLAYER_OGG_BUFFER_SIZE);
        ogg_sync_wrote(&_oggState.oggSyncState, bytesRead);
        return bytesRead >= VIDEO_PLAYER_OGG_BUFFER_SIZE;
    }
    else
    {
        assert(_fileStream);
        std::size_t bytesRead = fread(buffer, 1, VIDEO_PLAYER_OGG_BUFFER_SIZE, _fileStream);
        ogg_sync_wrote(&_oggState.oggSyncState, bytesRead);
        return bytesRead >= VIDEO_PLAYER_OGG_BUFFER_SIZE;
    }
}

bool VideoPlayer::readHeaders()
{
    _oggState.reset();

    ogg_sync_init(&_oggState.oggSyncState);
    th_comment_init(&_oggState.theoraComment);
    th_info_init(&_oggState.theoraInfo);
    vorbis_info_init(&_oggState.vorbisInfo);
    vorbis_comment_init(&_oggState.vorbisComment);

    ogg_page page = { 0 };
    ogg_packet packet = { 0 };

    // Find streams
    bool foundStreams = false;
    while (!foundStreams)
    {
        bool success = readStream();
        if (!success)
        {
            break;
        }

        while (ogg_sync_pageout(&_oggState.oggSyncState, &page) > 0)
        {
            if (!ogg_page_bos(&page))
            {
                _oggState.pagein(&page);
                foundStreams = true;
                break;
            }

            ogg_stream_state test;
            ogg_stream_init(&test, ogg_page_serialno(&page));
            ogg_stream_pagein(&test, &page);
            ogg_stream_packetout(&test, &packet);

            if (!_oggState.hasTheora && th_decode_headerin(&_oggState.theoraInfo, &_oggState.theoraComment, &_oggState.theoraSetup, &packet) >= 0)
            {
                _oggState.hasTheora = true;
                memcpy(&_oggState.theoraStreamState, &test, sizeof(test));
            }
            else if (!_oggState.hasVorbis && vorbis_synthesis_headerin(&_oggState.vorbisInfo, &_oggState.vorbisComment, &packet) >= 0)
            {
                _oggState.hasVorbis = true;
                memcpy(&_oggState.vorbisStreamState, &test, sizeof(test));
            }
            else
            {
                ogg_stream_clear(&test);
            }
        }
    }

    int theoraHeadersCount = 1, vorbisHeadersCount = 1, result = 0;
    while ((_oggState.hasTheora && theoraHeadersCount < 3) || (_oggState.hasVorbis && vorbisHeadersCount < 3))
    {
        while (_oggState.hasTheora && theoraHeadersCount < 3 && (result = ogg_stream_packetout(&_oggState.theoraStreamState, &packet)))
        {
            if (result == -1)
                continue;
            result = th_decode_headerin(&_oggState.theoraInfo, &_oggState.theoraComment, &_oggState.theoraSetup, &packet);
            if (result < 0)
            {
                log("Invalid Theora stream\n");
                _oggState.hasTheora = false;
                break;
            }
            assert(result != 0); // 0 means we got to the data stream, wrong
            theoraHeadersCount++;
        }
        while (_oggState.hasVorbis && vorbisHeadersCount < 3 && (result = ogg_stream_packetout(&_oggState.vorbisStreamState, &packet)))
        {
            if (result == -1)
                continue;
            if (vorbis_synthesis_headerin(&_oggState.vorbisInfo, &_oggState.vorbisComment, &packet))
            {
                log("Invalid Vorbis stream\n");
                _oggState.hasVorbis = false;
                break;
            }
            vorbisHeadersCount++;
        }
        result = ogg_sync_pageout(&_oggState.oggSyncState, &page);
        if (result > 0)
        {
            _oggState.pagein(&page);
        }
        else
        {
            bool success = readStream();
            assert(success);
        }
    }

    if (_oggState.hasTheora)
    {
        initVideo();
        _oggState.theoraDecoder = th_decode_alloc(&_oggState.theoraInfo, _oggState.theoraSetup);
    }
    else
    {
        th_info_clear(&_oggState.theoraInfo);
        th_comment_clear(&_oggState.theoraComment);
    }
    th_setup_free(_oggState.theoraSetup);

    if (_oggState.hasVorbis)
    {
        if (_oggState.vorbisInfo.channels != 2)
        {
            log("Only supports stereo audio");
            vorbis_info_clear(&_oggState.vorbisInfo);
            vorbis_comment_clear(&_oggState.vorbisComment);
            _oggState.hasVorbis = false;
        }
        else
        {
            initAudio();
            vorbis_synthesis_init(&_oggState.vorbisDSPState, &_oggState.vorbisInfo);
            vorbis_block_init(&_oggState.vorbisDSPState, &_oggState.vorbisBlock);
        }
    }
    else
    {
        vorbis_info_clear(&_oggState.vorbisInfo);
        vorbis_comment_clear(&_oggState.vorbisComment);
    }

    if (_oggState.hasTheora)
    {
        log("video: %dx%d %.2f fps", _oggState.theoraInfo.frame_width, _oggState.theoraInfo.frame_height, (double)_oggState.theoraInfo.fps_numerator / _oggState.theoraInfo.fps_denominator);
    }
    else
    {
        log("no video");
    }
    if (_oggState.hasVorbis)
    {
        log("audio: %d hz %d channels", _oggState.vorbisInfo.rate, _oggState.vorbisInfo.channels);
    }
    else
    {
        log("no audio");
    }

    return _oggState.hasTheora || _oggState.hasVorbis;
}

void VideoPlayer::initAudio()
{
}

void VideoPlayer::shutAudio()
{
}

void VideoPlayer::pushAudioFrame()
{
    float **pcm = nullptr;
    int numSamples = 0;
    {
        std::unique_lock<std::mutex> lock(_oggMutex);
        numSamples = vorbis_synthesis_pcmout(&_oggState.vorbisDSPState, &pcm);
        vorbis_synthesis_read(&_oggState.vorbisDSPState, numSamples);
    }
    if (numSamples == 0) // There are no PCM samples
    {
        return;
    }

    std::unique_ptr<AudioFrame> frame(new AudioFrame());
    frame->numSamples = numSamples;
    frame->samplesL.reset(new float[numSamples]);
    memcpy(frame->samplesL.get(), pcm[0], sizeof(float) * numSamples);
    frame->samplesR.reset(new float[numSamples]);
    memcpy(frame->samplesR.get(), pcm[1], sizeof(float) * numSamples);

    std::unique_lock<std::mutex> lock(_audioMutex);
    _audioFrames.push_back(std::move(frame));
    _audioTotalSamples += numSamples;
}

void VideoPlayer::initVideo()
{
    for (int i = 0; i < 3; i++)
    {
        _bufferTextures[i] = nullptr;
    }
}

void VideoPlayer::shutVideo()
{
}

void VideoPlayer::pushVideoFrame()
{
    // Skip frames that won't be shown anyway
    if (_videoTime < _timeLastFrame)
    {
        return;
    }

    // Consume frame in any case
    th_ycbcr_buffer yuv;
    {
        std::unique_lock<std::mutex> lock(_oggMutex);
        th_decode_ycbcr_out(_oggState.theoraDecoder, yuv);
    }

    // Push new frame
    {
        std::unique_ptr<VideoFrame> frame(new VideoFrame());
        frame->time = _videoTime;
        for (int i = 0; i < 3; i++)
        {
            frame->buffer[i].width = yuv[i].width;
            frame->buffer[i].height = yuv[i].height;
            frame->buffer[i].stride = yuv[i].stride;
            frame->buffer[i].bytes = yuv[i].height * yuv[i].stride;
            frame->data[i].reset(new uint8_t[frame->buffer[i].bytes]);
            memcpy(frame->data[i].get(), yuv[i].data, frame->buffer[i].bytes);
        }
        _videoMutex.lock();
        _videoFrames.push_back(std::move(frame));
        _videoMutex.unlock();
    }
}

void VideoPlayer::processVideo()
{
    if (!_processVideo)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(_videoMutex);

    VideoFrames::iterator frameToDisplayIterator = _videoFrames.end();

    for (VideoFrames::iterator frameIterator = _videoFrames.begin(); frameIterator != _videoFrames.end();)
    {
        VideoFrame* frame = frameIterator->get();
        if (frame->time < _timeLastFrame)
        {
            frameIterator = _videoFrames.erase(frameIterator);
            continue;
        }
        else if (frame->time < _timer)
        {
            frameToDisplayIterator = frameIterator;
            break;
        }
        frameIterator++;
    }

    if (frameToDisplayIterator != _videoFrames.end())
    {
        VideoFrame* frameToDisplay = frameToDisplayIterator->get();

        _glMutex.lock();

        for (int i = 0; i < 3; i++)
        {
            const VideoBuffer& buffer = frameToDisplay->buffer[i];
            if (_bufferTextures[i] == nullptr)
            {
                _bufferTextures[i] = _createTextureCallback(_userData, i, buffer.stride, buffer.height);
            }
            if (_bufferTextures[i] != nullptr)
            {
                _uploadTextureCallback(_userData, i, frameToDisplay->data[i].get(), buffer.bytes);
            }
        }

        _timeLastFrame = frameToDisplay->time;
        _videoFrames.erase(frameToDisplayIterator);

        _glMutex.unlock();
    }

    _processVideo = false;
}

void VideoPlayer::getFrameSize(int& width, int& height, int& x, int& y)
{
    if (_state == VideoPlayerState::Initialized)
    {
        width = 0;
        height = 0;
        x = 0;
        y = 0;
    }
    else
    {
        width = _oggState.theoraInfo.pic_width;
        height = _oggState.theoraInfo.pic_height;
        x = _oggState.theoraInfo.pic_x;
        y = _oggState.theoraInfo.pic_y;
    }
}

void VideoPlayer::getAudioInfo(int& channels, int& frequency)
{
    if (_state == VideoPlayerState::Initialized)
    {
        channels = 0;
        frequency = 0;
    }
    else
    {
        channels = _oggState.vorbisInfo.channels;
        frequency = _oggState.vorbisInfo.rate;
    }
}

void VideoPlayer::pcmRead(float* data, int numSamples)
{
    std::unique_lock<std::mutex> lock(_audioMutex);
    if (_audioFrames.empty())
    {
        assert(_audioTotalSamples == 0);
        memset(data, 0, sizeof(float) * numSamples);
        return;
    }

    int samplesWritten = 0;
    while (samplesWritten < numSamples)
    {
        float* output = &data[samplesWritten * 2];

        AudioFrame *frame = _audioFrames.front().get();
        float *inputL = &frame->samplesL.get()[frame->samplesRead];
        float *inputR = &frame->samplesR.get()[frame->samplesRead];
        const int samplesLeftToWrite = numSamples - samplesWritten;
        const int samplesLeftToRead = frame->numSamples - frame->samplesRead;
        const int samplesToWrite = samplesLeftToWrite < samplesLeftToRead ? samplesLeftToWrite : samplesLeftToRead;
        for(int i = 0; i < samplesToWrite; i++)
        {
            output[i * 2 + 0] = inputL[i];
            output[i * 2 + 1] = inputR[i];
        }
        frame->samplesRead += samplesToWrite;
        if (frame->samplesRead == frame->numSamples)
        {
            _audioFrames.pop_front();
        }
        samplesWritten += samplesToWrite;
        if (_audioFrames.empty())
        {
            break;
        }
    }
    if (samplesWritten < numSamples)
    {
        memset(&data[samplesWritten * 2], 0, sizeof(float) * (numSamples - samplesWritten));
    }
    _audioTotalSamples -= samplesWritten;
}

void VideoPlayer::threadDecode(VideoPlayer* p)
{
    bool endOfFile = false;
    bool videoReady = false, audioReady = false;

    ogg_page page = {};
    ogg_packet packet = {};
    ogg_int64_t videoGranule = 0;

    p->_timer = 0;
    p->_timerStart = 0;
    p->_timeLastFrame = 0;

    p->_videoFrames.clear();
    p->_videoTime = 0;
    p->_processVideo = false;

    p->_audioFrames.clear();
    {
        int32_t audioBufferSize = p->_audioBufferSize;
        if (p->_getValueCallback(p->_userData, VideoPlayerValueType::AudioBufferSize, &audioBufferSize))
        {
            p->_audioBufferSize = audioBufferSize * VIDEO_PLAYER_AUDIO_BUFFERED_FRAMES;
        }
    }
    p->_audioTotalSamples = 0;

    bool streaming = false;

    while (true)
    {
        if (p->_state == VideoPlayerState::Paused)
        {
            if (!p->waitPause())
            {
                p->log("break pause\n");
                break;
            }
            assert(p->_state != VideoPlayerState::Paused);
        }
        if (p->_state != VideoPlayerState::Playing)
        {
            p->log("break state\n");
            break;
        }

        int cachedAudioTotalSamples = 0;
        {
            std::unique_lock<std::mutex> lock(p->_audioMutex);
            cachedAudioTotalSamples = p->_audioTotalSamples;
        }
        int cachedVideoNumFrames = 0;
        {
            std::unique_lock<std::mutex> lock(p->_videoMutex);
            cachedVideoNumFrames = p->_videoFrames.size();
        }

        // Decode audio
        p->_oggMutex.lock();
        while (p->_oggState.hasVorbis && !audioReady && cachedAudioTotalSamples < p->_audioBufferSize)
        {
            int result = ogg_stream_packetout(&p->_oggState.vorbisStreamState, &packet);
            if (result > 0)
            {
                result = vorbis_synthesis(&p->_oggState.vorbisBlock, &packet);
                if (result == 0)
                {
                    result = vorbis_synthesis_blockin(&p->_oggState.vorbisDSPState, &p->_oggState.vorbisBlock);
                    if (result == 0)
                    {
                        audioReady = true;
                    }
                }
            }
            else
            {
                break;
            }
        }
        p->_oggMutex.unlock();

        // Decode video
        p->_oggMutex.lock();
        while (p->_oggState.hasTheora && !videoReady && cachedVideoNumFrames < VIDEO_PLAYER_VIDEO_BUFFERED_FRAMES)
        {
            int result = ogg_stream_packetout(&p->_oggState.theoraStreamState, &packet);
            if (result > 0)
            {
                int result = th_decode_packetin(p->_oggState.theoraDecoder, &packet, &videoGranule);
                if (result == 0)
                {
                    p->_videoTime = th_granule_time(p->_oggState.theoraDecoder, videoGranule);
                    videoReady = true;
                }
            }
            else
            {
                break;
            }
        }
        p->_oggMutex.unlock();

        const bool flushed = cachedAudioTotalSamples == 0 && cachedVideoNumFrames == 0;
        if (endOfFile && !audioReady && !videoReady && flushed)
        {
            p->log("break eof\n");
            p->setState(VideoPlayerState::Stopped);
            break;
        }

        if ((p->_oggState.hasVorbis && !audioReady && cachedAudioTotalSamples < p->_audioBufferSize)
            || (p->_oggState.hasTheora && !videoReady && cachedVideoNumFrames < VIDEO_PLAYER_VIDEO_BUFFERED_FRAMES))
        {
            p->_oggMutex.lock();
            bool success = p->readStream();
            while (success && ogg_sync_pageout(&p->_oggState.oggSyncState, &page) > 0)
            {
                p->_oggState.pagein(&page);
            }
            p->_oggMutex.unlock();
            if (!success)
            {
                endOfFile = true;
            }
        }

        if (!streaming && (!p->_oggState.hasVorbis || audioReady) && (!p->_oggState.hasTheora || videoReady))
        {
            streaming = true;
            if (p->_getValueCallback != nullptr)
            {
                double timerCurrent = 0;
                if (p->_getValueCallback(p->_userData, VideoPlayerValueType::AudioTime, &timerCurrent))
                {
                    p->_timerStart = timerCurrent;
                }
            }
            p->_timer = 0.0;
        }

        if (streaming && audioReady)
        {
            p->pushAudioFrame();
            audioReady = false;
        }
        if (streaming && videoReady)
        {
            p->pushVideoFrame();
            videoReady = false;
        }

        p->_processVideo = true;
    }

    if (p->_oggState.hasTheora)
    {
        p->shutVideo();
    }
    if (p->_oggState.hasVorbis)
    {
        p->shutAudio();
    }

    p->close();
}

void VideoPlayer::launchDecode()
{
    if (_threadDecode.joinable())
    {
        waitDecode();
    }
    _threadDecode = std::thread(threadDecode, this);
}

void VideoPlayer::waitDecode()
{
    log("join...");
    _threadDecode.join();
    log("joined\n");
}

void VideoPlayer::signalPause()
{

}

void VideoPlayer::cancelPause()
{

}

bool VideoPlayer::waitPause()
{
    return true;
}

bool VideoPlayer::open()
{
    if (!readHeaders())
    {
        log("failed to read headers");
        close();
        return false;
    }

    setState(VideoPlayerState::Ready);
    return true;
}

void VideoPlayer::close()
{
    log("closing...");
    {
        std::unique_lock<std::mutex> lock(_audioMutex);
        _audioFrames.clear();
        _audioTotalSamples = 0;
    }
    {
        std::unique_lock<std::mutex> lock(_videoMutex);
        _videoFrames.clear();
    }

    if (_fileStream != nullptr)
    {
        fclose(_fileStream);
        _fileStream = nullptr;
    }
    if (_zipStream.isOpen())
    {
        _zipStream.close();
    }
    log("closed\n");
}

bool VideoPlayer::openCallback(VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    close();

    if (dataCallback == nullptr || createTextureCallback == nullptr || uploadTextureCallback == nullptr)
    {
        return false;
    }

    _dataCallback = dataCallback;
    _createTextureCallback = createTextureCallback;
    _uploadTextureCallback = uploadTextureCallback;

    return open();
}

bool VideoPlayer::openFile(std::string filePath, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback)
{
    close();

    if (filePath.empty())
    {
        return false;
    }
    if (createTextureCallback == nullptr || uploadTextureCallback == nullptr)
    {
        return false;
    }

    log("opening file: %s", filePath.c_str());

    static std::regex jarRegex("jar:file://(.+?)!/(.+?)", std::regex_constants::icase);
    std::smatch jarMatch;
    if (std::regex_match(filePath, jarMatch, jarRegex))
    {
        log("zip format", filePath.c_str());
        const std::string jarFile = jarMatch[1].str();
        const std::string assetFile = jarMatch[2].str();
        if (!_zipStream.open(jarFile.c_str(), assetFile.c_str()))
        {
            log("failed to open zip: %s, entry %s", jarFile.c_str(), assetFile.c_str());
            return false;
        }
    }
    else
    {
        log("normal file", filePath.c_str());
        _fileStream = fopen(filePath.c_str(), "rb");
        if (_fileStream == nullptr)
        {
            return false;
        }
    }

    _dataCallback = nullptr;
    _createTextureCallback = createTextureCallback;
    _uploadTextureCallback = uploadTextureCallback;

    return open();
}

void VideoPlayer::play()
{
    if (_state != VideoPlayerState::Ready)
    {
        return;
    }

    log("play\n");
    setState(VideoPlayerState::Playing);

    launchDecode();
}

void VideoPlayer::pause()
{
    setState(VideoPlayerState::Paused);
}

void VideoPlayer::resume()
{
    setState(VideoPlayerState::Playing);
    signalPause();
}

void VideoPlayer::stop()
{
    log("stopping...");
    if (_threadDecode.joinable())
    {
        setState(VideoPlayerState::Stopped);
        cancelPause();
        waitDecode();
    }
    log("stopped\n");
}

void VideoPlayer::update(float timeStep)
{
    if (_state == VideoPlayerState::Playing)
    {
        std::unique_lock<std::mutex> lock(_videoMutex);
        if (_getValueCallback != nullptr)
        {
            double timerCurrent = 0;
            if (_getValueCallback(_userData, VideoPlayerValueType::AudioTime, &timerCurrent))
            {
                _timer = timerCurrent - _timerStart;
            }
            else
            {
                _timer += timeStep;
            }
        }
        else
        {
            _timer += timeStep;
        }
    }
    processVideo();
}
