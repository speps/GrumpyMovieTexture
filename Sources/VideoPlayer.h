#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <chrono>

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>

#define VIDEO_PLAYER_OGG_BUFFER_SIZE 4096
#define VIDEO_PLAYER_AUDIO_BUFFER_SIZE 4096
#define VIDEO_PLAYER_VIDEO_BUFFERED_FRAMES 3
#define MALLOC malloc
#define FREE free
#if WIN32
#include <windows.h>
#define LOG(...) { char tmp[1024]; sprintf_s(tmp, sizeof(tmp), __VA_ARGS__); OutputDebugStringA(tmp); }
#else
#define LOG(...)
#endif

enum class VideoPlayerState
{
    None,
    Initialized,
    Ready,
    Playing,
    Paused,
    Stopped
};

typedef void (*VideoStatusCallback)(void* userData, VideoPlayerState newState);
typedef void (*VideoTimeCallback)(void* userData, double* time);
typedef bool (*VideoDataCallback)(void* userData, uint8_t* outData, uint32_t bytesMax, uint32_t* bytesRead);
typedef void* (*VideoCreateTextureCallback)(void* userData, int index, int width, int height);
typedef void (*VideoUploadTextureCallback)(void* userData, int index, uint8_t* data, int size);

class VideoPlayer
{
private:
    void* _userData;
    VideoPlayerState _state;
    FILE* _fileStream;
    VideoStatusCallback _statusCallback;
    VideoTimeCallback _timeCallback;
    VideoDataCallback _dataCallback;
    VideoCreateTextureCallback _createTextureCallback;
    VideoUploadTextureCallback _uploadTextureCallback;
    std::mutex _pauseMutex;
    std::condition_variable _pauseEvent;
    std::atomic<bool> _processVideo;

    double _timer, _timerStart, _timeLastFrame;
    void* _bufferTextures[3];

    struct VideoBuffer
    {
        int width, height, stride, bytes;
    };
    typedef VideoBuffer YCbCrBuffer[3];

    struct VideoFrame
    {
        double time;
        std::unique_ptr<uint8_t[]> data[3];
        YCbCrBuffer buffer;

        VideoFrame()
            : time(0.0)
        {
        }
    };
    typedef std::list<std::unique_ptr<VideoFrame>> VideoFrames;
    VideoFrames _videoFrames;

    std::mutex _videoMutex, _glMutex;
    double _videoTime;

    struct AudioFrame
    {
        std::unique_ptr<float[]> samplesL, samplesR;
        int numSamples, samplesRead;

        AudioFrame()
            : samplesL(nullptr), samplesR(nullptr), numSamples(0), samplesRead(0)
        {
        }
    };
    typedef std::list<std::unique_ptr<AudioFrame>> AudioFrames;
    AudioFrames _audioFrames;

    std::mutex _audioMutex;
    int _audioTotalSamples;

    struct OggState
    {
        // ogg
        ogg_sync_state   oggSyncState;
        // theora
        bool             hasTheora;
        ogg_stream_state theoraStreamState;
        th_info          theoraInfo;
        th_setup_info*   theoraSetup;
        th_dec_ctx*      theoraDecoder;
        th_comment       theoraComment;
        // vorbis
        bool             hasVorbis;
        ogg_stream_state vorbisStreamState;
        vorbis_info      vorbisInfo;
        vorbis_dsp_state vorbisDSPState;
        vorbis_block     vorbisBlock;
        vorbis_comment   vorbisComment;

        OggState()
        {
            reset();
        }
        void reset()
        {
            memset(&oggSyncState, 0, sizeof(OggState));
        }
        void pagein(ogg_page *page)
        {
            if (hasTheora)
                ogg_stream_pagein(&theoraStreamState, page);
            if (hasVorbis)
                ogg_stream_pagein(&vorbisStreamState, page);
        }
    };
    OggState _oggState;

    void destroy();
    void setState(VideoPlayerState newState);

    bool open();

    bool readStream(); // returns false at end of file
    bool readHeaders();

    void initAudio();
    void shutAudio();
    void pushAudioFrame();

    void initVideo();
    void shutVideo();
    void pushVideoFrame();

    std::thread _threadDecode;
    static void threadDecode(VideoPlayer* p);
    void launchDecode();
    void waitDecode();

    void signalPause();
    void cancelPause();
    bool waitPause();
public:
    VideoPlayer(void* userData, VideoStatusCallback statusCallback, VideoTimeCallback timeCallback);
    virtual ~VideoPlayer();

    VideoPlayerState state() const
    { return _state; }

    bool openCallback(VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback);
    bool openFile(std::string filePath, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback);

    void play();
    bool isPlaying() const
    {
        return _state == VideoPlayerState::Playing;
    }

    bool isPaused() const
    {
        return _state == VideoPlayerState::Paused;
    }
    void pause();
    void resume();

    void stop();
    bool isStopped() const
    {
        return _state == VideoPlayerState::Stopped;
    }

    void update(float timeStep);
    void processVideo();

    void getFrameSize(int& width, int& height, int& x, int& y);
    void getAudioInfo(int& channels, int& frequency);
    void pcmRead(float* data, int numSamples);
};
