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
    public bool withAudio = false;
    public RenderTexture renderTexture;

    enum State
    {
        None,
        TogglePlayer,
        TogglePlaying,
    }
    private State nextState = State.TogglePlayer;

    void Update()
    {
        var player = GetComponent<VideoPlayer>();
        if (nextState != State.None)
        {
            if (nextState == State.TogglePlayer)
            {
                if (player == null)
                {
                    if (withAudio)
                    {
                        var audioSource = gameObject.AddComponent<AudioSource>();
                        audioSource.playOnAwake = false;
                        Debug.Log("Created with audio");
                    }
                    player = gameObject.AddComponent<VideoPlayer>();
                    player.renderTexture = renderTexture;
                }
                else
                {
                    Destroy(player);
                    player = null;
                }
            }
            else if (nextState == State.TogglePlaying)
            {
                bool isPlaying = player.IsPlaying;
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
            nextState = State.None;
        }
        var text = GameObject.FindGameObjectWithTag("DebugText").GetComponent<UnityEngine.UI.Text>();
        text.text = player == null ? "null" : (player.IsPlaying ? "Playing" : "Stopped");

        //foreach (var touch in Input.touches)
        //{
        //    if (touch.phase == TouchPhase.Ended)
        //    {
        //        nextState = State.TogglePlaying;
        //    }
        //}
    }

    void OnGUI()
    {
        float scale = (float)Screen.width / 640;
        GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(scale, scale, 1));

        var player = GetComponent<VideoPlayer>();
        withAudio = GUILayout.Toggle(withAudio, "With Audio");
        if (GUILayout.Button(player == null ? "Create" : "Destroy"))
        {
            nextState = State.TogglePlayer;
        }
        if (player == null)
        {
            return;
        }
        for (int i = 0; i < videos.Length; i++)
        {
            bool current = selected == i;
            if (GUILayout.Toggle(current, videos[i]))
            {
                selected = i;
            }
        }
        if (GUILayout.Button(player.IsPlaying ? "Stop" : "Play"))
        {
            nextState = State.TogglePlaying;
        }
    }
}
