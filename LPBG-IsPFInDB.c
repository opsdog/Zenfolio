/*
  returns true if PhotoId is in PFInDB list
*/

/*
#undef DEBUG
#define DEBUG
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#include "Zenfolio.h"

#include "LoadPhotosByGallery.h"


#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif


int IsPFInDB(int PhotoId, struct pflags *PFInDB_Top)
{
  struct pflags *PFInDB_Current;

#ifdef DEBUG
  printf("==>  IsPFInDB:  entry %d\n",PhotoId);
#endif

  PFInDB_Current = PFInDB_Top;
  while ( PFInDB_Current != NULL ) {
#ifdef DEBUG
    printf("==>  IsPFInDB:    comparing %d to %d\n",
	   PhotoId, PFInDB_Current->pf_PID);
#endif
    if ( PFInDB_Current->pf_PID == PhotoId )
      return(1);
    PFInDB_Current = PFInDB_Current->pf_Next;
  }

#ifdef DEBUG
  printf("==>  IsPFInDB:  end exit\n");
#endif

  return(0);

}
