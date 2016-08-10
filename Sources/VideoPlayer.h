#pragma once

#include <stdlib.h>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>

#define VIDEO_PLAYER_NUM_BUFFERS 2
#define VIDEO_PLAYER_OGG_BUFFER_SIZE 4096
#define VIDEO_PLAYER_AUDIO_BUFFER_SIZE 4096
#define VIDEO_PLAYER_AUDIO_BUFFERED_FRAMES 3
#define VIDEO_PLAYER_VIDEO_BUFFERED_FRAMES 3
#define MALLOC malloc
#define FREE free
#if WIN32
#include <windows.h>
#define LOG(...) { char tmp[256]; sprintf_s(tmp, sizeof(tmp), __VA_ARGS__); OutputDebugStringA(tmp); }
#else
#define LOG(...)
#endif

typedef bool (__stdcall *VideoDataCallback)(uint8_t* outData, uint32_t bytesMax, uint32_t* bytesRead);
typedef void* (__stdcall *VideoCreateTextureCallback)(int index, int width, int height);
typedef void (__stdcall *VideoUploadTextureCallback)(int index, uint8_t* data, int size);

namespace VideoPlayerState
{
    enum Value
    {
        None,
        Initialized,
        Ready,
        Playing,
        Paused,
        Stopped
    };
}

class VideoPlayer
{
private:
    VideoPlayerState::Value _state;
    VideoDataCallback _dataCallback;
    VideoCreateTextureCallback _createTextureCallback;
    VideoUploadTextureCallback _uploadTextureCallback;
    std::mutex _pauseMutex;
    std::condition_variable _pauseEvent;
    bool _processVideo;

    double _timer, _timeLastFrame;
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
        int bytesRead;

        AudioFrame()
            :samplesL(nullptr), samplesR(nullptr), bytesRead(0)
        {

        }

        AudioFrame *pNext, *pPrev;
    };
    typedef std::list<std::unique_ptr<AudioFrame>> AudioFrames;
    AudioFrames _audioFrames;

    std::mutex _audioMutex;
    int _audioBufferCount, _audioBufferWritten, _audioBufferSize;
    std::unique_ptr<float[]> _audioBufferL, _audioBufferR;

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
    VideoPlayer();
    virtual ~VideoPlayer();

    VideoPlayerState::Value state() const
    { return _state; }

    bool open(VideoDataCallback dataCallback, VideoCreateTextureCallback createTextureCallback, VideoUploadTextureCallback uploadTextureCallback);

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
};
