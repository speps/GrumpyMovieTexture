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

 CPU capability detection for x86 processors.
  Originally written by Rudolf Marek.

 function:
  last mod: $Id$

 ********************************************************************/

#include "neoncpu.h"

#if !defined(OC_NEON)
ogg_uint32_t oc_cpu_flags_get(void){
  return 0;
}
#else
ogg_uint32_t oc_cpu_flags_get(void){
  return OC_CPU_NEON;
}
#endif