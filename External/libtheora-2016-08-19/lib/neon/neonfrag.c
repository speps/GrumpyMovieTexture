/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id$

 ********************************************************************/
#include <string.h>
#include <arm_neon.h>
#include "../internal.h"

void oc_frag_copy_neon(unsigned char *_dst,const unsigned char *_src,int _ystride){
  int i;
  for(i=8;i-->0;){
    vst1_u8(_dst, vld1_u8(_src));
    _dst+=_ystride;
    _src+=_ystride;
  }
}

/*Copies the fragments specified by the lists of fragment indices from one
   frame to another.
  _dst_frame:     The reference frame to copy to.
  _src_frame:     The reference frame to copy from.
  _ystride:       The row stride of the reference frames.
  _fragis:        A pointer to a list of fragment indices.
  _nfragis:       The number of fragment indices to copy.
  _frag_buf_offs: The offsets of fragments in the reference frames.*/
void oc_frag_copy_list_neon(unsigned char *_dst_frame,
 const unsigned char *_src_frame,int _ystride,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,const ptrdiff_t *_frag_buf_offs){
  ptrdiff_t fragii;
  for(fragii=0;fragii<_nfragis;fragii++){
    ptrdiff_t frag_buf_off;
    frag_buf_off=_frag_buf_offs[_fragis[fragii]];
    oc_frag_copy_neon(_dst_frame+frag_buf_off,
     _src_frame+frag_buf_off,_ystride);
  }
}
