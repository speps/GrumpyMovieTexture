set CURL=%~dp0\curl.exe
pushd "GrumpyProject\Assets\GrumpyMovieTexture\Plugins"
%CURL% -Lk https://dl.bintray.com/speps/GrumpyMovieTexture/Plugins/x64/GrumpyMovieTexture.dll -o x64/GrumpyMovieTexture.dll
%CURL% -Lk https://dl.bintray.com/speps/GrumpyMovieTexture/Plugins/x86/GrumpyMovieTexture.dll -o x86/GrumpyMovieTexture.dll
%CURL% -Lk https://dl.bintray.com/speps/GrumpyMovieTexture/Plugins/Android/libs/armeabi-v7a/libGrumpyMovieTexture.so -o Android/libs/armeabi-v7a/libGrumpyMovieTexture.so
%CURL% -Lk https://dl.bintray.com/speps/GrumpyMovieTexture/Plugins/iOS/libGrumpyMovieTexture.a -o iOS/libGrumpyMovieTexture.a
%CURL% -Lk https://dl.bintray.com/speps/GrumpyMovieTexture/Plugins/GrumpyMovieTexture.bundle/Contents/MacOS/GrumpyMovieTexture -o GrumpyMovieTexture.bundle/Contents/MacOS/GrumpyMovieTexture
popd
pause