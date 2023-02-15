#ifndef COMMON_JPEG_H
#define COMMON_JPEG_H

#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>

void jpeg_mem_src_own(j_decompress_ptr cinfo, void *buffer, long nbytes);

#endif
