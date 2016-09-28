using UnityEngine;
using System.Collections;

public class MovieController : MonoBehaviour
{
    private static string[] videos = new string[]
    {
        "av_sync_test.ogv",
        "big_buck_bunny_720p_stereo.ogv",
    };
    public int selected = 0;
    public RenderTexture renderTexture;

    void OnGUI()
    {
        var player = GetComponent<VideoPlayer>();
        if (GUILayout.Button(player == null ? "Create" : "Destroy"))
        {
            if (player == null)
            {
                var audioSource = gameObject.AddComponent<AudioSource>();
                audioSource.playOnAwake = false;
                player = gameObject.AddComponent<VideoPlayer>();
                player.renderTexture = renderTexture;
            }
            else
            {
                Destroy(player);
                player = null;
            }
        }
        if (player == null)
        {
            return;
        }
        bool isPlaying = player.IsPlaying;
        for (int i = 0; i < videos.Length; i++)
        {
            bool current = selected == i;
            if (GUILayout.Toggle(current, videos[i]))
            {
                selected = i;
            }
        }
        if (GUILayout.Button(isPlaying ? "Stop" : "Play"))
        {
            if (isPlaying)
            {
                player.Stop();
            }
            else
            {
                player.Open(videos[selected]);
                player.Play();
            }
        }
    }
}
