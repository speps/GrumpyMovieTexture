set(LIBOGG_VERSION libogg-1.3.1)
set(LIBOGG_SOURCES
    bitwise.c
    framing.c
)
prefix_path(LIBOGG_SOURCES "${PROJECT_SOURCE_DIR}/External/${LIBOGG_VERSION}/src")

set(LIBTHEORA_VERSION libtheora-2016-08-19)
set(LIBTHEORA_SOURCES
    apiwrapper.c
    bitpack.c
    decapiwrapper.c
    decinfo.c
    decode.c
    dequant.c
    fragment.c
    huffdec.c
    idct.c
    info.c
    internal.c
    quant.c
    state.c
    encoder_disabled.c
)
if(IOS)
    list(APPEND LIBTHEORA_SOURCES
        neon/neoncpu.c
        neon/neonstate.c
        neon/neonidct.c
        neon/neonfrag.c
    )
endif()
prefix_path(LIBTHEORA_SOURCES "${PROJECT_SOURCE_DIR}/External/${LIBTHEORA_VERSION}/lib")

set(LIBVORBIS_VERSION libvorbis-1.3.3)
set(LIBVORBIS_SOURCES
    mdct.c smallft.c block.c envelope.c window.c lsp.c
    lpc.c analysis.c synthesis.c psy.c info.c
    floor1.c floor0.c
    res0.c mapping0.c registry.c codebook.c sharedbook.c
    lookup.c bitrate.c
    envelope.h lpc.h lsp.h codebook.h misc.h psy.h
    masking.h os.h mdct.h smallft.h highlevel.h
    registry.h scales.h window.h lookup.h lookup_data.h
    codec_internal.h backends.h bitrate.h
)
prefix_path(LIBVORBIS_SOURCES "${PROJECT_SOURCE_DIR}/External/${LIBVORBIS_VERSION}/lib")

set(GMT_SOURCES
    VideoPlayer.cpp
    VideoPlayer.h
    VideoPlayerInterface.cpp
    ${LIBOGG_SOURCES}
    ${LIBTHEORA_SOURCES}
    ${LIBVORBIS_SOURCES}
)

if(IOS)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++> -O3 -mfpu=neon -mfloat-abi=softfp)
    add_library(GrumpyMovieTexture STATIC ${GMT_SOURCES})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_library(GrumpyMovieTexture MODULE ${GMT_SOURCES})
    set_target_properties(GrumpyMovieTexture PROPERTIES BUNDLE 1)
else()
    add_library(GrumpyMovieTexture SHARED ${GMT_SOURCES})
endif()

target_include_directories(GrumpyMovieTexture PRIVATE
    ${PROJECT_SOURCE_DIR}/External/${LIBOGG_VERSION}/include
    ${PROJECT_SOURCE_DIR}/External/${LIBTHEORA_VERSION}/include
    ${PROJECT_SOURCE_DIR}/External/${LIBVORBIS_VERSION}/include
)
target_link_libraries(GrumpyMovieTexture ${GMT_LIBS})
