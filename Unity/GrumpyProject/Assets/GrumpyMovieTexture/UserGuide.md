# GrumpyMovieTexture

## Video setup

* All movies have to be in Ogg format with Theora for video and Vorbis for audio (if sound is needed).
* Place movie inside `StreamingAssets` folder inside the `Assets` folder

## Setup from a new project

* Import `GrumpyMovieTexture.unityPackage` or install from Asset Store
* In the files selection, only select the `GrumpyMovieTexture` folder
* Create a new empty GameObject if needed
* Add `Video Player` script
* If you need sound, add "Audio Source" component above the "Video Player"
* Set `Streaming Assets Path` to one of your video file names

## Usage from script

* `var player = GetComponent<VideoPlayer>();` to access the player
* With that, you can :
	* `Play()` : start the movie
	* `IsPlaying` : start the movie
	* `Stop()` : stop the movie
