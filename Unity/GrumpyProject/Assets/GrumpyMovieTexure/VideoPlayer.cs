using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using System.IO;

public class VideoPlayer : MonoBehaviour
{
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate bool DataCallback(IntPtr data, int bytesMax, out int bytesRead);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate IntPtr CreateTextureCallback(int index, int width, int height);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void UploadTextureCallback(int index, IntPtr data, int size);

#if UNITY_IPHONE && !UNITY_EDITOR
    public const string DLLName = "__Internal";
#else
    public const string DLLName = "GrumpyMovieTexture";
#endif

    [DllImport(DLLName)]
    private static extern IntPtr VPCreate();

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
    private static extern bool VPOpen(IntPtr player, DataCallback dataCallback, CreateTextureCallback createTextureCallback, UploadTextureCallback uploadTextureCallback);

    [DllImport(DLLName)]
    private static extern void VPUpdate(IntPtr player, float timeStep);

    [DllImport(DLLName)]
    private static extern void VPGetFrameSize(IntPtr player, out int width, out int height, out int x, out int y);

    public string streamingAssetsFileName;
    public RenderTexture renderTexture;

    IntPtr player;
    FileStream asset;
    byte[] assetBuffer;
    int assetOffset;
    Material material;
    Rect sourceRect;
    Texture2D[] textures = new Texture2D[3];
    DataCallback dataCallback;
    CreateTextureCallback createTextureCallback;
    UploadTextureCallback uploadTextureCallback;

    void OnEnable()
    {
        player = VPCreate();
        var shader = Shader.Find("Hidden/GrumpyConvertRGB");
        material = new Material(shader);

        dataCallback = (IntPtr data, int bytesMax, out int bytesRead) =>
        {
            if (assetBuffer == null || assetBuffer.Length < bytesMax)
            {
                assetBuffer = new byte[bytesMax];
            }
            bytesRead = asset.Read(assetBuffer, 0, bytesMax);
            Marshal.Copy(assetBuffer, 0, data, bytesRead);
            return bytesRead == bytesMax;
        };
        createTextureCallback = (index, width, height) =>
        {
            textures[index] = new Texture2D(width, height, TextureFormat.Alpha8, false, true);
            return textures[index].GetNativeTexturePtr();
        };
        uploadTextureCallback = (index, data, size) =>
        {
            textures[index].LoadRawTextureData(data, size);
            textures[index].Apply();
        };

        OpenResource();
    }

    void OnDisable()
    {
        asset.Close();
        VPDestroy(player);
        player = IntPtr.Zero;
    }

    void OpenResource()
    {
        asset = File.OpenRead(Path.Combine(Application.streamingAssetsPath, streamingAssetsFileName));
        VPOpen(player, dataCallback, createTextureCallback, uploadTextureCallback);
        int width, height, x, y;
        VPGetFrameSize(player, out width, out height, out x, out y);
        sourceRect = new Rect(x, y, width, height);
    }

    void Update()
    {
        VPUpdate(player, Time.deltaTime);
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

    void OnGUI()
    {
        bool isPlaying = VPIsPlaying(player);
        if (GUILayout.Button(isPlaying ? "Stop" : "Play"))
        {
            if (isPlaying)
            {
                VPStop(player);
            }
            else
            {
                if (VPIsStopped(player))
                {
                    OpenResource();
                }
                VPPlay(player);
            }
        }
    }
}
