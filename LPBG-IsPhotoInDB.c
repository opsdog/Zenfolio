/*
  returns true if PhotoId is in PhotoInDB list
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


int IsPhotoInDB(int PhotoId, struct photo *PhotoInDB_Top)
{
  struct photo *PhotoInDB_Current;

#ifdef DEBUG
  printf("==>  IsPhotoInDB:  entry %d\n",PhotoId);
#endif

  PhotoInDB_Current = PhotoInDB_Top;
  while ( PhotoInDB_Current != NULL ) {
#ifdef DEBUG
    printf("==>  IsPhotoInDB:    comparing %d to %d\n",
	   PhotoId, PhotoInDB_Current->ph_ID);
#endif
    if ( PhotoInDB_Current->ph_ID == PhotoId )
      return(1);
    PhotoInDB_Current = PhotoInDB_Current->ph_Next;
  }

#ifdef DEBUG
  printf("==>  IsPhotoInDB:  end exit\n");
#endif

  return(0);

}
