using UnityEngine;
using System.Collections;

[RequireComponent(typeof(VideoPlayer))]
public class MovieController : MonoBehaviour
{
    void OnGUI()
    {
        var player = GetComponent<VideoPlayer>();
        bool isPlaying = player.IsPlaying;
        if (GUILayout.Button(isPlaying ? "Stop" : "Play"))
        {
            if (isPlaying)
            {
                player.Stop();
            }
            else
            {
                player.Play();
            }
        }
    }
}
