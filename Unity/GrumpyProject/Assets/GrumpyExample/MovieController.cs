using UnityEngine;
using System.Collections;

[RequireComponent(typeof(VideoPlayer))]
public class MovieController : MonoBehaviour
{
    private static string[] videos = new string[]
    {
        "av_sync_test.ogv",
        "big_buck_bunny_720p_stereo.ogv"
    };
    public int selected = 0;

    void OnGUI()
    {
        var player = GetComponent<VideoPlayer>();
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
