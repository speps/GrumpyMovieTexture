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
    vst1_u8(_dst,vld1_u8(_src));
    _dst+=_ystride;
    _src+=_ystride;
  }
}

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

void oc_frag_recon_intra_neon(unsigned char *_dst,int _ystride,
 const ogg_int16_t _residue[64]){
  int16x8_t v0=vdupq_n_s16(0);
  int16x8_t v128=vdupq_n_s16(128);
  int16x8_t v255=vdupq_n_s16(255);
  int i;
  for(i=0;i<8;i++){
    int16x8_t v=vld1q_s16(&_residue[i*8]);
    v=vaddq_s16(v,v128);
    v=vminq_s16(v,v255);
    v=vmaxq_s16(v,v0);
    vst1_u8(_dst,vmovn_u16(vreinterpretq_u16_s16(v)));
    _dst+=_ystride;
  }
}

void oc_frag_recon_inter_neon(unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]){
  int16x8_t v0=vdupq_n_s16(0);
  int16x8_t v255=vdupq_n_s16(255);
  int i;
  for(i=0;i<8;i++){
    int16x8_t v=vld1q_s16(&_residue[i*8]);
    uint8x8_t s=vld1_u8(_src);
    v=vaddq_s16(v,vreinterpretq_s16_u16(vmovl_u8(s)));
    v=vminq_s16(v,v255);
    v=vmaxq_s16(v,v0);
    vst1_u8(_dst,vmovn_u16(vreinterpretq_u16_s16(v)));
    _dst+=_ystride;
    _src+=_ystride;
  }
}

void oc_frag_recon_inter2_neon(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t _residue[64]){
  int16x8_t v0=vdupq_n_s16(0);
  int16x8_t v255=vdupq_n_s16(255);
  int i;
  for(i=0;i<8;i++){
    int16x8_t v=vld1q_s16(&_residue[i*8]);
    uint8x8_t s1=vld1_u8(_src1);
    uint8x8_t s2=vld1_u8(_src2);
    uint8x8_t s=vhadd_u8(s1,s2); // (s1+s2)>>1
    v=vaddq_s16(v,vreinterpretq_s16_u16(vmovl_u8(s)));
    v=vminq_s16(v,v255);
    v=vmaxq_s16(v,v0);
    vst1_u8(_dst,vmovn_u16(vreinterpretq_u16_s16(v)));
    _dst+=_ystride;
    _src1+=_ystride;
    _src2+=_ystride;
  }
}
