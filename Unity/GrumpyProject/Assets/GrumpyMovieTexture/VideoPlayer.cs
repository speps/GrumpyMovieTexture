using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using System.IO;

public class VideoPlayer : MonoBehaviour
{
    public class MonoPInvokeCallbackAttribute : System.Attribute
    {
        private Type type;
        public MonoPInvokeCallbackAttribute(Type t) { type = t; }
    }

    public delegate bool DataCallback(IntPtr userData, IntPtr data, int bytesMax, out int bytesRead);
    public delegate IntPtr CreateTextureCallback(IntPtr userData, int index, int width, int height);
    public delegate void UploadTextureCallback(IntPtr userData, int index, IntPtr data, int size);

#if UNITY_IPHONE && !UNITY_EDITOR
    public const string DLLName = "__Internal";
#else
    public const string DLLName = "GrumpyMovieTexture";
#endif

    [DllImport(DLLName)]
    private static extern IntPtr VPCreate(IntPtr userData);

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

#if UNITY_IOS
    [DllImport ("__Internal")]
    private static extern void AudioSessionSetup();
#endif
    
    public string streamingAssetsFileName;
    public RenderTexture renderTexture;

    GCHandle handle;
    IntPtr player;
    Material material;
    Rect sourceRect;
    Texture2D[] textures = new Texture2D[3];

    void OnEnable()
    {
        handle = GCHandle.Alloc(this);
        player = VPCreate(GCHandle.ToIntPtr(handle));
        var shader = Shader.Find("Hidden/GrumpyConvertRGB");
        material = new Material(shader);

        OpenResource();
    }

    void OnDisable()
    {
        VPDestroy(player);
        player = IntPtr.Zero;
        handle.Free();
    }

    void OpenResource()
    {
        var filePath = Path.Combine(Application.streamingAssetsPath, streamingAssetsFileName);
        VPOpenFile(player, filePath, OnCreateTextureCallback, OnUploadTextureCallback);
        int width, height, x, y;
        VPGetFrameSize(player, out width, out height, out x, out y);
        sourceRect = new Rect(x, y, width, height);
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

    void FixedUpdate()
    {
        VPUpdate(player, Time.fixedDeltaTime);
        if (textures[0] != null)
        {
            RenderTexture.active = renderTexture;
            GL.PushMatrix();
            GL.LoadPixelMatrix(0, renderTexture.width, renderTexture.height, 0);
            material.SetTexture("_MainCbTex", textures[1]);
            material.SetTexture("_MainCrTex", textures[2]);
            var sourceSize = new Vector2(textures[0].width, textures[0].height);
            Graphics.DrawTexture(new Rect(0, 0, renderTexture.width, renderTexture.height), textures[0],
                new Rect(sourceRect.x / sourceSize.x, sourceRect.y / sourceSize.y, sourceRect.width / sourceSize.x, sourceRect.height / sourceSize.y), 0, 0, 0, 0, material);
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
#if UNITY_IOS
        if (Application.platform == RuntimePlatform.IPhonePlayer)
        {
            AudioSessionSetup();
        }
#endif
        VPPlay(player);
    }

    public bool IsPlaying
    {
        get { return VPIsPlaying(player); }
    }

    public void Stop()
    {
        VPStop(player);
    }
}
