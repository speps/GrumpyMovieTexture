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

void oc_frag_recon_intra_neon(unsigned char *_dst,int _ystride,
 const ogg_int16_t _residue[64]){
  // int i;
  // for(i=0;i<8;i++){
  //   int j;
  //   for(j=0;j<8;j++)_dst[j]=OC_CLAMP255(_residue[i*8+j]+128);
  //   _dst+=_ystride;
  // }
  asm volatile (
    "VMOV.I16  Q0, #128\n\t"
    "VLDMIA %[_residue],  {D16-D31}  ; D16= 3333222211110000 etc ; 9(8) cycles\n\t"
    "VQADD.S16 Q8, Q8, Q0\n\t"
    "VQADD.S16 Q9, Q9, Q0\n\t"
    "VQADD.S16 Q10,Q10,Q0\n\t"
    "VQADD.S16 Q11,Q11,Q0\n\t"
    "VQADD.S16 Q12,Q12,Q0\n\t"
    "VQADD.S16 Q13,Q13,Q0\n\t"
    "VQADD.S16 Q14,Q14,Q0\n\t"
    "VQADD.S16 Q15,Q15,Q0\n\t"
    "VQMOVUN.S16 D16,Q8  ; D16= 7766554433221100   ; 1 cycle\n\t"
    "VQMOVUN.S16 D17,Q9  ; D17= FFEEDDCCBBAA9988   ; 1 cycle\n\t"
    "VQMOVUN.S16 D18,Q10 ; D18= NNMMLLKKJJIIHHGG   ; 1 cycle\n\t"
    "VST1.64 {D16},[%[_dst]@64], %[_ystride]\n\t"
    "VQMOVUN.S16 D19,Q11 ; D19= VVUUTTSSRRQQPPOO   ; 1 cycle\n\t"
    "VST1.64 {D17},[%[_dst]@64], %[_ystride]\n\t"
    "VQMOVUN.S16 D20,Q12 ; D20= ddccbbaaZZYYXXWW   ; 1 cycle\n\t"
    "VST1.64 {D18},[%[_dst]@64], %[_ystride]\n\t"
    "VQMOVUN.S16 D21,Q13 ; D21= llkkjjiihhggffee   ; 1 cycle\n\t"
    "VST1.64 {D19},[%[_dst]@64], %[_ystride]\n\t"
    "VQMOVUN.S16 D22,Q14 ; D22= ttssrrqqppoonnmm   ; 1 cycle\n\t"
    "VST1.64 {D20},[%[_dst]@64], %[_ystride]\n\t"
    "VQMOVUN.S16 D23,Q15 ; D23= !!@@zzyyxxwwvvuu   ; 1 cycle\n\t"
    "VST1.64 {D21},[%[_dst]@64], %[_ystride]\n\t"
    "VST1.64 {D22},[%[_dst]@64], %[_ystride]\n\t"
    "VST1.64 {D23},[%[_dst]@64], %[_ystride]\n\t"
    :
    :
    : "q0", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
  )
}

// oc_frag_recon_intra_neon PROC
//   ; r0 =       unsigned char *_dst
//   ; r1 =       int            _ystride
//   ; r2 = const ogg_int16_t    _residue[64]
//   VMOV.I16  Q0, #128
//   VLDMIA  r2,  {D16-D31}  ; D16= 3333222211110000 etc ; 9(8) cycles
//   VQADD.S16 Q8, Q8, Q0
//   VQADD.S16 Q9, Q9, Q0
//   VQADD.S16 Q10,Q10,Q0
//   VQADD.S16 Q11,Q11,Q0
//   VQADD.S16 Q12,Q12,Q0
//   VQADD.S16 Q13,Q13,Q0
//   VQADD.S16 Q14,Q14,Q0
//   VQADD.S16 Q15,Q15,Q0
//   VQMOVUN.S16 D16,Q8  ; D16= 7766554433221100   ; 1 cycle
//   VQMOVUN.S16 D17,Q9  ; D17= FFEEDDCCBBAA9988   ; 1 cycle
//   VQMOVUN.S16 D18,Q10 ; D18= NNMMLLKKJJIIHHGG   ; 1 cycle
//   VST1.64 {D16},[r0@64], r1
//   VQMOVUN.S16 D19,Q11 ; D19= VVUUTTSSRRQQPPOO   ; 1 cycle
//   VST1.64 {D17},[r0@64], r1
//   VQMOVUN.S16 D20,Q12 ; D20= ddccbbaaZZYYXXWW   ; 1 cycle
//   VST1.64 {D18},[r0@64], r1
//   VQMOVUN.S16 D21,Q13 ; D21= llkkjjiihhggffee   ; 1 cycle
//   VST1.64 {D19},[r0@64], r1
//   VQMOVUN.S16 D22,Q14 ; D22= ttssrrqqppoonnmm   ; 1 cycle
//   VST1.64 {D20},[r0@64], r1
//   VQMOVUN.S16 D23,Q15 ; D23= !!@@zzyyxxwwvvuu   ; 1 cycle
//   VST1.64 {D21},[r0@64], r1
//   VST1.64 {D22},[r0@64], r1
//   VST1.64 {D23},[r0@64], r1
//   MOV PC,R14
//   ENDP

void oc_frag_recon_inter_neon(unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]){
  // int i;
  // for(i=0;i<8;i++){
  //   int j;
  //   for(j=0;j<8;j++)_dst[j]=OC_CLAMP255(_residue[i*8+j]+_src[j]);
  //   _dst+=_ystride;
  //   _src+=_ystride;
  // }
}

// oc_frag_recon_inter_neon PROC
//   ; r0 =       unsigned char *_dst
//   ; r1 = const unsigned char *_src
//   ; r2 =       int            _ystride
//   ; r3 = const ogg_int16_t    _residue[64]
//   VLDMIA  r3, {D16-D31} ; D16= 3333222211110000 etc ; 9(8) cycles
//   VLD1.64 {D0}, [r1], r2
//   VLD1.64 {D2}, [r1], r2
//   VMOVL.U8  Q0, D0  ; Q0 = __77__66__55__44__33__22__11__00
//   VLD1.64 {D4}, [r1], r2
//   VMOVL.U8  Q1, D2  ; etc
//   VLD1.64 {D6}, [r1], r2
//   VMOVL.U8  Q2, D4
//   VMOVL.U8  Q3, D6
//   VQADD.S16 Q8, Q8, Q0
//   VLD1.64 {D0}, [r1], r2
//   VQADD.S16 Q9, Q9, Q1
//   VLD1.64 {D2}, [r1], r2
//   VQADD.S16 Q10,Q10,Q2
//   VLD1.64 {D4}, [r1], r2
//   VQADD.S16 Q11,Q11,Q3
//   VLD1.64 {D6}, [r1], r2
//   VMOVL.U8  Q0, D0
//   VMOVL.U8  Q1, D2
//   VMOVL.U8  Q2, D4
//   VMOVL.U8  Q3, D6
//   VQADD.S16 Q12,Q12,Q0
//   VQADD.S16 Q13,Q13,Q1
//   VQADD.S16 Q14,Q14,Q2
//   VQADD.S16 Q15,Q15,Q3
//   VQMOVUN.S16 D16,Q8
//   VQMOVUN.S16 D17,Q9
//   VQMOVUN.S16 D18,Q10
//   VST1.64 {D16},[r0@64], r2
//   VQMOVUN.S16 D19,Q11
//   VST1.64 {D17},[r0@64], r2
//   VQMOVUN.S16 D20,Q12
//   VST1.64 {D18},[r0@64], r2
//   VQMOVUN.S16 D21,Q13
//   VST1.64 {D19},[r0@64], r2
//   VQMOVUN.S16 D22,Q14
//   VST1.64 {D20},[r0@64], r2
//   VQMOVUN.S16 D23,Q15
//   VST1.64 {D21},[r0@64], r2
//   VST1.64 {D22},[r0@64], r2
//   VST1.64 {D23},[r0@64], r2
//   MOV PC,R14
//   ENDP

void oc_frag_recon_inter2_neon(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t _residue[64]){
  // int i;
  // for(i=0;i<8;i++){
  //   int j;
  //   for(j=0;j<8;j++)_dst[j]=OC_CLAMP255(_residue[i*8+j]+(_src1[j]+_src2[j]>>1));
  //   _dst+=_ystride;
  //   _src1+=_ystride;
  //   _src2+=_ystride;
  // }
}

// oc_frag_recon_inter2_neon PROC
//   ; r0 =       unsigned char *_dst
//   ; r1 = const unsigned char *_src1
//   ; r2 = const unsigned char *_src2
//   ; r3 =       int            _ystride
//   LDR r12,[r13]
//   ; r12= const ogg_int16_t    _residue[64]
//   VLDMIA  r12,{D16-D31}
//   VLD1.64 {D0}, [r1], r3
//   VLD1.64 {D4}, [r2], r3
//   VLD1.64 {D1}, [r1], r3
//   VLD1.64 {D5}, [r2], r3
//   VHADD.U8  Q2, Q0, Q2  ; Q2 = FFEEDDCCBBAA99887766554433221100
//   VLD1.64 {D2}, [r1], r3
//   VLD1.64 {D6}, [r2], r3
//   VMOVL.U8  Q0, D4    ; Q0 = __77__66__55__44__33__22__11__00
//   VLD1.64 {D3}, [r1], r3
//   VMOVL.U8  Q2, D5    ; etc
//   VLD1.64 {D7}, [r2], r3
//   VHADD.U8  Q3, Q1, Q3
//   VQADD.S16 Q8, Q8, Q0
//   VQADD.S16 Q9, Q9, Q2
//   VLD1.64 {D0}, [r1], r3
//   VMOVL.U8  Q1, D6
//   VLD1.64 {D4}, [r2], r3
//   VMOVL.U8  Q3, D7
//   VLD1.64 {D1}, [r1], r3
//   VQADD.S16 Q10,Q10,Q1
//   VLD1.64 {D5}, [r2], r3
//   VQADD.S16 Q11,Q11,Q3
//   VLD1.64 {D2}, [r1], r3
//   VHADD.U8  Q2, Q0, Q2
//   VLD1.64 {D6}, [r2], r3
//   VLD1.64 {D3}, [r1], r3
//   VMOVL.U8  Q0, D4
//   VLD1.64 {D7}, [r2], r3
//   VMOVL.U8  Q2, D5
//   VHADD.U8  Q3, Q1, Q3
//   VQADD.S16 Q12,Q12,Q0
//   VQADD.S16 Q13,Q13,Q2
//   VMOVL.U8  Q1, D6
//   VMOVL.U8  Q3, D7
//   VQADD.S16 Q14,Q14,Q1
//   VQADD.S16 Q15,Q15,Q3
//   VQMOVUN.S16 D16,Q8
//   VQMOVUN.S16 D17,Q9
//   VQMOVUN.S16 D18,Q10
//   VST1.64 {D16},[r0@64], r3
//   VQMOVUN.S16 D19,Q11
//   VST1.64 {D17},[r0@64], r3
//   VQMOVUN.S16 D20,Q12
//   VST1.64 {D18},[r0@64], r3
//   VQMOVUN.S16 D21,Q13
//   VST1.64 {D19},[r0@64], r3
//   VQMOVUN.S16 D22,Q14
//   VST1.64 {D20},[r0@64], r3
//   VQMOVUN.S16 D23,Q15
//   VST1.64 {D21},[r0@64], r3
//   VST1.64 {D22},[r0@64], r3
//   VST1.64 {D23},[r0@64], r3
//   MOV PC,R14
//   ENDP
