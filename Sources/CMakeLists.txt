set(GMT_SOURCES
	VideoPlayer.cpp
	VideoPlayer.h
	VideoPlayerInterface.cpp
)
set(GMT_LIBS
	ogg
	theora
	vorbis
)

if(IOS)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
else()
	set(GMT_LIBS ${GMT_LIBS} opengl32.lib)
endif()

if (IOS)
	add_library(GrumpyMovieTexture STATIC ${GMT_SOURCES})
else()
	add_library(GrumpyMovieTexture SHARED ${GMT_SOURCES})
endif()
target_compile_definitions(GrumpyMovieTexture PRIVATE GLEW_STATIC)
target_link_libraries(GrumpyMovieTexture ${GMT_LIBS})
