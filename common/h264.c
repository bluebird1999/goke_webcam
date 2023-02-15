/*
 * header
 */
//system header
#include <stdio.h>
#include <pthread.h>
#include <syscall.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <malloc.h>
//program header

//server header
#include "h264.h"

/*
 * static
 */
//variable

//function;



/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

int h264_is_iframe(char flag) {
    flag = flag & 0x1f;
    if( (flag == 7) || (flag == 8) || (flag == 5)) {
        return 1;
    } else {
        return 0;
    }
}

int h264_is_pframe(char *p)
{
	return (*p == 0x41) ? 1 : 0;
}

int h264_read_nalu(unsigned char *pPack, unsigned int nPackLen, unsigned int offSet, nalu_unit_t *pnalu_unit)
{
  int i = offSet;
  while (i < nPackLen) {
	  if (pPack[i++] == 0x00 && pPack[i++] == 0x00 && pPack[i++] == 0x00 && pPack[i++] == 0x01) {
		  int pos = i;
		  while (pos < nPackLen) {
			  if (pPack[pos++] == 0x00 && pPack[pos++] == 0x00 && pPack[pos++] == 0x00 && pPack[pos++] == 0x01)
				  break;
		  }
		  if (pos == nPackLen)
			  pnalu_unit->size = pos - i;
		  else
			  pnalu_unit->size = (pos - 4) - i;

		  pnalu_unit->type = pPack[i] & 0x1f;
		  pnalu_unit->data = (unsigned char *)&pPack[i];
		  return (pnalu_unit->size + i - offSet);
	  }
  }
  return 0;
}
