set(LIBOGG_VERSION libogg-1.3.1)
set(LIBOGG_SOURCES
	bitwise.c
	framing.c
)
prefix_path(LIBOGG_SOURCES "${LIBOGG_VERSION}/src")
add_library(ogg STATIC ${LIBOGG_SOURCES})
target_include_directories(ogg PUBLIC ${LIBOGG_VERSION}/include)

set(LIBTHEORA_VERSION libtheora-1.1.1)
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
prefix_path(LIBTHEORA_SOURCES "${LIBTHEORA_VERSION}/lib")
add_library(theora STATIC ${LIBTHEORA_SOURCES})
target_include_directories(theora PUBLIC ${LIBTHEORA_VERSION}/include)
target_link_libraries(theora ogg)

set(LIBVORBIS_VERSION libvorbis-1.3.3)
include_directories(${LIBVORBIS_VERSION}/include)
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
prefix_path(LIBVORBIS_SOURCES "${LIBVORBIS_VERSION}/lib")
add_library(vorbis STATIC ${LIBVORBIS_SOURCES})
target_include_directories(vorbis PUBLIC ${LIBVORBIS_VERSION}/include)
target_link_libraries(vorbis ogg)
