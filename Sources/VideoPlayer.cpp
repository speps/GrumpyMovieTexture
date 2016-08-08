#include "VideoPlayer.h"
#include "RenderAPI.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

VideoPlayer::VideoPlayer()
    : _state(VideoPlayerState::Initialized), _timer(0.0), _timeLastFrame(0.0),
    _audioBufferCount(0), _audioBufferWritten(0), _audioBufferSize(0), _videoTime(0.0),
    _dataCallback(nullptr), _processVideo(false)
{
}

VideoPlayer::~VideoPlayer()
{
    destroy();
}

void VideoPlayer::destroy()
{
    stop();
    _dataCallback = nullptr;
    _textureCallback = nullptr;
}

bool VideoPlayer::readStream()
{
    uint8_t* buffer = (uint8_t*)ogg_sync_buffer(&_oggState.oggSyncState, VIDEO_PLAYER_OGG_BUFFER_SIZE);
    uint32_t bytesRead = 0;
    bool success = _dataCallback(buffer, VIDEO_PLAYER_OGG_BUFFER_SIZE, &bytesRead);
    ogg_sync_wrote(&_oggState.oggSyncState, bytesRead);
    return success;
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
                LOG("Invalid Theora stream\n");
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
                LOG("Invalid Vorbis stream\n");
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
            LOG("Only supports stereo audio");
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

    return _oggState.hasTheora || _oggState.hasVorbis;
}

//FMOD_RESULT F_CALLBACK VideoPlayer::pcmReadCallback(FMOD_SOUND *sound, void *data, unsigned int datalen)
//{
//    void *userData = NULL;
//    FMOD_Sound_GetUserData(sound, &userData);
//    VideoPlayer *p = static_cast<VideoPlayer*>(userData);
//
//    p->_audioMutex.waitLock();
//    if (p->_audioFrames.empty())
//    {
//        p->_audioMutex.release();
//        return FMOD_ERR_NOTREADY;
//    }
//    
//    signed short *output = (signed short*)data;
//
//    AudioFrame *frame = p->_audioFrames.head();
//    float *inputL = (float*)(((char*)frame->samplesL) + frame->bytesRead);
//    float *inputR = (float*)(((char*)frame->samplesR) + frame->bytesRead);
//    for(unsigned int i = 0; i < datalen >> sizeof(short); i++)
//    {
//        output[i * 2 + 0] = (signed short)(clamp(inputL[i], -1.0f, 1.0f) * 32767.0f);
//        output[i * 2 + 1] = (signed short)(clamp(inputR[i], -1.0f, 1.0f) * 32767.0f);
//    }
//    frame->bytesRead += datalen;
//    if (frame->bytesRead == p->_audioBufferSize)
//    {
//        p->_audioFrames.remove(frame);
//    }
//    p->_audioMutex.release();
//
//    return FMOD_OK;
//}

void VideoPlayer::initAudio()
{
}

void VideoPlayer::shutAudio()
{
}

void VideoPlayer::pushAudioFrame()
{
    assert(_audioBufferCount == 0 && _audioBufferWritten == _audioBufferSize);

    std::unique_ptr<AudioFrame> frame(new AudioFrame());
    frame->samplesL.reset(new float[_audioBufferSize]);
    memcpy(frame->samplesL.get(), _audioBufferL.get(), _audioBufferSize);
    frame->samplesR.reset(new float[_audioBufferSize]);
    memcpy(frame->samplesR.get(), _audioBufferR.get(), _audioBufferSize);

    _audioMutex.lock();
    _audioFrames.push_back(std::move(frame));
    _audioMutex.unlock();

    _audioBufferCount = VIDEO_PLAYER_AUDIO_BUFFER_SIZE;
    _audioBufferWritten = 0;
}

void VideoPlayer::initVideo()
{
    int width = _oggState.theoraInfo.frame_width;
    int height = _oggState.theoraInfo.frame_height;
    int subWidth = width;
    int subHeight = height;

    switch (_oggState.theoraInfo.pixel_fmt)
    {
    case TH_PF_420:
        subHeight >>= 1;
    case TH_PF_422:
        subWidth >>= 1;
    }

    _bufferTextures[0] = _textureCallback(0, width, height);
    _bufferTextures[1] = _textureCallback(1, subWidth, subHeight);
    _bufferTextures[2] = _textureCallback(2, subWidth, subHeight);
}

void VideoPlayer::shutVideo()
{
    
}

void VideoPlayer::pushVideoFrame()
{
    // Consume frame in any case
    th_ycbcr_buffer yuv;
    th_decode_ycbcr_out(_oggState.theoraDecoder, yuv);

    LOG("video time %f time %f\n", _videoTime, _timer);

    // Push new frame
    //if (_videoTime >= _timer)
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

    LOG("PROCESS total=%lld tslf=%f t=%f d=%p (t=%f)\n", _videoFrames.size(), _timeLastFrame, _timer,
        frameToDisplayIterator != _videoFrames.end() ? frameToDisplayIterator->get() : 0,
        frameToDisplayIterator != _videoFrames.end() ? frameToDisplayIterator->get()->time : 0);

    if (frameToDisplayIterator != _videoFrames.end())
    {
        VideoFrame* frameToDisplay = frameToDisplayIterator->get();

        _glMutex.lock();

        for (int i = 0; i < 3; i++)
        {
            const VideoBuffer& buffer = frameToDisplay->buffer[i];
            extern RenderAPI* s_CurrentAPI;
            LOG("Update %dx%d %p\n", buffer.width, buffer.height, frameToDisplay->data[i].get());
            s_CurrentAPI->UpdateTexture(_bufferTextures[i], buffer.width, buffer.height, buffer.stride, frameToDisplay->data[i].get());
        }

        _timeLastFrame = frameToDisplay->time;
        _videoFrames.erase(frameToDisplayIterator);

        _glMutex.unlock();
    }

    _processVideo = false;
}

void VideoPlayer::threadDecode(VideoPlayer* p)
{
    bool endOfFile = false;
    bool videoReady = false, audioReady = false;

    ogg_page page = {};
    ogg_packet packet = {};
    ogg_int64_t videoGranule = 0;

    p->_timer = 0;
    p->_timeLastFrame = 0;
    p->_videoTime = 0;
    p->_processVideo = false;
    p->_audioBufferCount = VIDEO_PLAYER_AUDIO_BUFFER_SIZE;
    p->_audioBufferWritten = 0;
    p->_audioBufferSize = sizeof(float) * VIDEO_PLAYER_AUDIO_BUFFER_SIZE; // Number of bytes per channel
    p->_audioBufferL.reset(new float[p->_audioBufferSize]);
    p->_audioBufferR.reset(new float[p->_audioBufferSize]);

    p->_videoFrames.clear();
    p->_audioFrames.clear();

    bool streaming = false;

    while (true)
    {
        if (p->_state == VideoPlayerState::Paused)
        {
            if (!p->waitPause())
            {
                break;
            }
            assert(p->_state != VideoPlayerState::Paused);
        }
        if (p->_state != VideoPlayerState::Playing)
        {
            break;
        }

        // Decode audio
        while (p->_oggState.hasVorbis && !audioReady && p->_audioFrames.size() < VIDEO_PLAYER_AUDIO_BUFFERED_FRAMES)
        {
            float **pcm = NULL;
            int result = vorbis_synthesis_pcmout(&p->_oggState.vorbisDSPState, &pcm);
            if (result > 0) // There are PCM samples
            {
                // Fill buffer normally or fill only what's left in it
                int samples = p->_audioBufferCount > result ? result : p->_audioBufferCount;
                if (samples > 0)
                {
                    int bytes = sizeof(float) * samples;
                    memcpy(((uint8_t*)p->_audioBufferL.get()) + p->_audioBufferWritten, pcm[0], bytes);
                    memcpy(((uint8_t*)p->_audioBufferR.get()) + p->_audioBufferWritten, pcm[1], bytes);
                    p->_audioBufferWritten += bytes;

                    p->_audioBufferCount -= samples;
                    vorbis_synthesis_read(&p->_oggState.vorbisDSPState, samples);
                }
                if (p->_audioBufferCount == 0)
                {
                    audioReady = true;
                }
            }
            else
            {
                result = ogg_stream_packetout(&p->_oggState.vorbisStreamState, &packet);
                if (result > 0)
                {
                    result = vorbis_synthesis(&p->_oggState.vorbisBlock, &packet);
                    if (result == 0)
                    {
                        vorbis_synthesis_blockin(&p->_oggState.vorbisDSPState, &p->_oggState.vorbisBlock);
                    }
                }
                else
                {
                    break;
                }
            }
        }

        // Decode video
        while (p->_oggState.hasTheora && !videoReady && p->_videoFrames.size() < VIDEO_PLAYER_VIDEO_BUFFERED_FRAMES)
        {
            int result = ogg_stream_packetout(&p->_oggState.theoraStreamState, &packet);
            if (result > 0)
            {
                int result = th_decode_packetin(p->_oggState.theoraDecoder, &packet, &videoGranule);
                if (result == 0)
                {
                    p->_videoTime = th_granule_time(p->_oggState.theoraDecoder, videoGranule);
                    ogg_int64_t frame = th_granule_frame(p->_oggState.theoraDecoder, videoGranule);
                    LOG("video time %f frame %lld\n", p->_videoTime, frame);
                    videoReady = true;
                }
            }
            else
            {
                break;
            }
        }

        if (endOfFile && !audioReady && !videoReady)
        {
            p->_state = VideoPlayerState::Stopped;
            break;
        }

        if ((p->_oggState.hasVorbis && !audioReady && p->_audioFrames.size() < VIDEO_PLAYER_AUDIO_BUFFERED_FRAMES)
            || (p->_oggState.hasTheora && !videoReady && p->_videoFrames.size() < VIDEO_PLAYER_VIDEO_BUFFERED_FRAMES))
        {
            bool success = p->readStream();
            while (success && ogg_sync_pageout(&p->_oggState.oggSyncState, &page) > 0)
            {
                p->_oggState.pagein(&page);
            }
            if (!success)
            {
                endOfFile = true;
            }
        }

        if (!streaming && (!p->_oggState.hasVorbis || audioReady) && (!p->_oggState.hasTheora || videoReady))
        {
            streaming = true;
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

        //p->processVideo();
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

    p->_audioBufferL = nullptr;
    p->_audioBufferR = nullptr;
}

void VideoPlayer::launchDecode()
{
    _threadDecode = std::thread(threadDecode, this);
}

void VideoPlayer::waitDecode()
{
    _threadDecode.join();
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

bool VideoPlayer::open(VideoDataCallback dataCallback, VideoCreateTextureCallback textureCallback)
{
    stop();

    if (dataCallback == nullptr || textureCallback == nullptr)
    {
        return false;
    }

    _dataCallback = dataCallback;
    _textureCallback = textureCallback;

    if (!readHeaders())
    {
        _state = VideoPlayerState::Stopped;
        return false;
    }

    _state = VideoPlayerState::Ready;
    return true;
}

void VideoPlayer::play()
{
    if (_state != VideoPlayerState::Ready)
    {
        return;
    }

    _state = VideoPlayerState::Playing;

    launchDecode();
}

void VideoPlayer::pause()
{
    _state = VideoPlayerState::Paused;
    if (_oggState.hasVorbis)
    {
        //_audioChannel->setPaused(true);
    }
}

void VideoPlayer::resume()
{
    if (_oggState.hasVorbis)
    {
        //_audioChannel->setPaused(false);
    }
    _state = VideoPlayerState::Playing;
    signalPause();
}

void VideoPlayer::stop()
{
    if (!_threadDecode.joinable())
    {
        return;
    }

    _state = VideoPlayerState::Stopped;
    cancelPause();
    waitDecode();

    _audioFrames.clear();
    _videoFrames.clear();
}

void VideoPlayer::update(float timeStep)
{
    if (_state == VideoPlayerState::Playing)
    {
        _timer += timeStep;
    }
    processVideo();
}

//void VideoPlayer::render(RenderTarget& rt)
//{
//    if (_state == VideoPlayerState::None)
//    {
//        return;
//    }
//
//    GraphicsDevice::instance().identity();
//    GraphicsDevice::instance().setRenderTarget(&rt);
//
//    if(_glMutex.tryLock())
//    {
//    if (_bufferTextures[0] != NULL && _bufferTextures[1] != NULL && _bufferTextures[2] != NULL)
//    {
//        GraphicsDevice::instance().apply(_shader);
//        GraphicsDevice::instance().clear(Color::Black);
//
//        GraphicsDevice::instance().paramFP(SName("bufferY")).set(*_bufferTextures[0]);
//        GraphicsDevice::instance().paramFP(SName("bufferCb")).set(*_bufferTextures[1]);
//        GraphicsDevice::instance().paramFP(SName("bufferCr")).set(*_bufferTextures[2]);
//
//        GraphicsDevice::instance().renderQuad(Matrix3f::Identity, BlendingMode::Normal, Color::White, rt.width(), rt.height());
//    }
//    }
//    _glMutex.release();
//
//    GraphicsDevice::instance().setRenderTarget(NULL);
//}
