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

#if !defined(_neon_neonint_H)
# define _neon_neonint_H (1)
# include "../internal.h"

# if defined(OC_NEON)
#  define oc_state_accel_init oc_state_accel_init_neon
#  define oc_frag_copy(_state,_dst,_src,_ystride) \
  oc_frag_copy_neon(_dst,_src,_ystride)
#  define oc_frag_copy_list(_state,_dst_frame,_src_frame,_ystride, \
 _fragis,_nfragis,_frag_buf_offs) \
  oc_frag_copy_list_neon(_dst_frame,_src_frame,_ystride, \
   _fragis,_nfragis,_frag_buf_offs)
#   define oc_frag_recon_intra(_state,_dst,_ystride,_residue) \
  oc_frag_recon_intra_neon(_dst,_ystride,_residue)
#   define oc_frag_recon_inter(_state,_dst,_src,_ystride,_residue) \
  oc_frag_recon_inter_neon(_dst,_src,_ystride,_residue)
#   define oc_frag_recon_inter2(_state,_dst,_src1,_src2,_ystride,_residue) \
  oc_frag_recon_inter2_neon(_dst,_src1,_src2,_ystride,_residue)
#   define oc_idct8x8(_state,_y,_x,_last_zzi) \
  oc_idct8x8_neon(_y,_x,_last_zzi)
#  define OC_STATE_USE_VTABLE (1)
# endif

# include "../state.h"
# include "neoncpu.h"

void oc_state_accel_init_neon(oc_theora_state *_state);

void oc_frag_copy_neon(unsigned char *_dst,
 const unsigned char *_src,int _ystride);
void oc_frag_copy_list_neon(unsigned char *_dst_frame,
 const unsigned char *_src_frame,int _ystride,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,const ptrdiff_t *_frag_buf_offs);
void oc_frag_recon_intra_neon(unsigned char *_dst,int _ystride,
 const ogg_int16_t *_residue);
void oc_frag_recon_inter_neon(unsigned char *_dst,const unsigned char *_src,
 int _ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter2_neon(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t *_residue);
void oc_idct8x8_neon(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi);

#endif
