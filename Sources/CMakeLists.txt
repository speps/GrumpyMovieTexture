set(GMT_SOURCES
	GLEW/glew.c
	PlatformBase.h
	RenderAPI.cpp
	RenderAPI.h
	RenderAPI_D3D11.cpp
	RenderAPI_D3D12.cpp
	RenderAPI_D3D9.cpp
	RenderAPI_OpenGL2.cpp
	RenderAPI_OpenGLCoreES.cpp
	VideoPlayer.cpp
	VideoPlayer.h
	VideoPlayerInterface.cpp
)
add_library(GrumpyMovieTexture SHARED ${GMT_SOURCES})
target_compile_definitions(GrumpyMovieTexture PRIVATE GLEW_STATIC)
target_link_libraries(GrumpyMovieTexture libogg libtheora libvorbis opengl32.lib)
