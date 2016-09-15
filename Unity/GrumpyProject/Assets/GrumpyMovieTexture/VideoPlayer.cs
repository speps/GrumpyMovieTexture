﻿using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using System.IO;

public class VideoPlayer : MonoBehaviour
{
    public class MonoPInvokeCallbackAttribute : System.Attribute
    {
        public Type type;
        public MonoPInvokeCallbackAttribute(Type t) { type = t; }
    }

    public enum State
    {
        None,
        Initialized,
        // When open has succeeded
        Ready,
        // When play or resume has suceeded
        Playing,
        // When pause has been called
        Paused,
        // When stop has been called or end of video
        Stopped
    }

    public enum ValueType
    {
        None,
        AudioTime, // double
        AudioBufferSize, // int32
    }

    public delegate void StateChangedCallback(State newState);
    public event StateChangedCallback OnStateChanged;

    private delegate void StatusCallback(IntPtr userData, State newState);
    private delegate bool GetValueCallback(IntPtr userData, ValueType type, IntPtr value);
    private delegate bool DataCallback(IntPtr userData, IntPtr data, int bytesMax, out int bytesRead);
    private delegate IntPtr CreateTextureCallback(IntPtr userData, int index, int width, int height);
    private delegate void UploadTextureCallback(IntPtr userData, int index, IntPtr data, int size);

#if UNITY_IPHONE && !UNITY_EDITOR
    public const string DLLName = "__Internal";
#else
    public const string DLLName = "GrumpyMovieTexture";
#endif

    [DllImport(DLLName)]
    private static extern IntPtr VPCreate(IntPtr userData, StatusCallback statusCallback, GetValueCallback getValueCallback);

    [DllImport(DLLName)]
    private static extern void VPDestroy(IntPtr player);

    [DllImport(DLLName)]
    private static extern void VPPlay(IntPtr player);

    [DllImport(DLLName)]
    private static extern bool VPIsPlaying(IntPtr player);

    [DllImport(DLLName)]
    private static extern void VPStop(IntPtr player);

    [DllImport(DLLName)]
    private static extern bool VPIsStopped(IntPtr player);

    [DllImport(DLLName)]
    private static extern bool VPOpenCallback(IntPtr player, DataCallback dataCallback, CreateTextureCallback createTextureCallback, UploadTextureCallback uploadTextureCallback);

    [DllImport(DLLName)]
    private static extern bool VPOpenFile(IntPtr player, string filePath, CreateTextureCallback createTextureCallback, UploadTextureCallback uploadTextureCallback);

    [DllImport(DLLName)]
    private static extern void VPUpdate(IntPtr player, float timeStep);

    [DllImport(DLLName)]
    private static extern void VPGetFrameSize(IntPtr player, out int width, out int height, out int x, out int y);

    [DllImport(DLLName)]
    private static extern void VPGetAudioInfo(IntPtr player, out int channels, out int frequency);

    [DllImport(DLLName)]
    private static extern void VPPCMRead(IntPtr player, float[] data, int numSamples);

    public string streamingAssetsFileName;
    public RenderTexture renderTexture;

    GCHandle handle;
    IntPtr player;
    Material material;
    Rect sourceRect;
    Texture2D[] textures = new Texture2D[3];
    AudioConfiguration currentAudioConfiguration, newAudioConfiguration;

    void OnEnable()
    {
        handle = GCHandle.Alloc(this);
        player = VPCreate(GCHandle.ToIntPtr(handle), OnStatusCallback, OnGetValueCallback);
        var shader = Shader.Find("Hidden/GrumpyConvertRGB");
        material = new Material(shader);

        OpenResource();
    }

    void OnDisable()
    {
        if (IsPlaying)
        {
            Stop();
        }
        VPDestroy(player);
        player = IntPtr.Zero;
        handle.Free();
    }

    public void Open(string fileName)
    {
        streamingAssetsFileName = fileName;
        OpenResource();
    }

    void OpenResource()
    {
        if (string.IsNullOrEmpty(streamingAssetsFileName))
        {
            return;
        }
        var filePath = Path.Combine(Application.streamingAssetsPath, streamingAssetsFileName);
        bool result = VPOpenFile(player, filePath, OnCreateTextureCallback, OnUploadTextureCallback);
        if (!result)
        {
            Debug.LogErrorFormat("Failed to open '{0}'", filePath);
            return;
        }
        int width, height, x, y;
        VPGetFrameSize(player, out width, out height, out x, out y);
        sourceRect = new Rect(x, y, width, height);
        int channels, frequency;
        VPGetAudioInfo(player, out channels, out frequency);
        currentAudioConfiguration = AudioSettings.GetConfiguration();
        newAudioConfiguration = currentAudioConfiguration;
        newAudioConfiguration.sampleRate = frequency;
        newAudioConfiguration.speakerMode = channels == 2 ? AudioSpeakerMode.Stereo : AudioSpeakerMode.Mono;
    }

    [MonoPInvokeCallback(typeof(StatusCallback))]
    static void OnStatusCallback(IntPtr userData, State newState)
    {
        GCHandle handle = GCHandle.FromIntPtr(userData);
        VideoPlayer player = (VideoPlayer)handle.Target;
        if (player.OnStateChanged != null)
        {
            player.OnStateChanged(newState);
        }
    }

    [MonoPInvokeCallback(typeof(GetValueCallback))]
    static bool OnGetValueCallback(IntPtr userData, ValueType type, IntPtr value)
    {
        if (type == ValueType.AudioTime)
        {
            Marshal.StructureToPtr(AudioSettings.dspTime, value, false);
            return true;
        }
        else if (type == ValueType.AudioBufferSize)
        {
            GCHandle handle = GCHandle.FromIntPtr(userData);
            VideoPlayer player = (VideoPlayer)handle.Target;
            Marshal.StructureToPtr(player.newAudioConfiguration.dspBufferSize, value, false);
            return true;
        }
        return false;
    }

    [MonoPInvokeCallback(typeof(CreateTextureCallback))]
    static IntPtr OnCreateTextureCallback(IntPtr userData, int index, int width, int height)
    {
        GCHandle handle = GCHandle.FromIntPtr(userData);
        VideoPlayer player = (VideoPlayer)handle.Target;
        player.textures[index] = new Texture2D(width, height, TextureFormat.Alpha8, false, true);
        return player.textures[index].GetNativeTexturePtr();
    }

    [MonoPInvokeCallback(typeof(UploadTextureCallback))]
    static void OnUploadTextureCallback(IntPtr userData, int index, IntPtr data, int size)
    {
        GCHandle handle = GCHandle.FromIntPtr(userData);
        VideoPlayer player = (VideoPlayer)handle.Target;
        player.textures[index].LoadRawTextureData(data, size);
        player.textures[index].Apply();
    }

    void Update()
    {
        VPUpdate(player, Time.unscaledDeltaTime);

        if (renderTexture != null)
        {
            RenderTexture.active = renderTexture;
            GL.PushMatrix();
            GL.LoadPixelMatrix(0, renderTexture.width, renderTexture.height, 0);
            GL.Clear(false, true, Color.black);
            if (textures[0] != null)
            {
                material.SetTexture("_MainCbTex", textures[1]);
                material.SetTexture("_MainCrTex", textures[2]);
                var sourceSize = new Vector2(textures[0].width, textures[0].height);
                Graphics.DrawTexture(
                    new Rect(
                        0,
                        0,
                        renderTexture.width,
                        renderTexture.height),
                    textures[0],
                    new Rect(
                        sourceRect.x / sourceSize.x,
                        sourceRect.y / sourceSize.y,
                        sourceRect.width / sourceSize.x,
                        sourceRect.height / sourceSize.y),
                        0, 0, 0, 0, material);
            }
            GL.PopMatrix();
            RenderTexture.active = null;
        }
    }

    public void Play()
    {
        if (VPIsStopped(player))
        {
            OpenResource();
        }
        if (!AudioSettings.Reset(newAudioConfiguration))
        {
            Debug.LogError("Problem setting audio settings");
        }
        else
        {
            var audioSource = GetComponent<AudioSource>();
            if (audioSource != null)
            {
                audioSource.Play();
            }
        }
        VPPlay(player);
    }

    public bool IsPlaying
    {
        get { return VPIsPlaying(player); }
    }

    public void Stop()
    {
        VPStop(player);
        var audioSource = GetComponent<AudioSource>();
        if (audioSource != null)
        {
            audioSource.Stop();
        }
        AudioSettings.Reset(currentAudioConfiguration);
    }

    void OnAudioFilterRead(float[] data, int channels)
    {
        //if (IsPlaying)
        {
            VPPCMRead(player, data, data.Length / channels);
        }
    }
}
