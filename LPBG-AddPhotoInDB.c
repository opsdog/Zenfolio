/*
  Adds keyword info to the list
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


struct photo *AddPhotoInDB(struct photo *PhotoInDB_Top, char *PhotoIdString)
{

  struct photo *PhotoInDB_New;

#ifdef DEBUG
  printf("==>  AddPhotoInDB:  entry %s\n",PhotoIdString);
#endif

  if ( PhotoInDB_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddPhotoInDB:    add first PhotoId\n");
#endif

    /*  build node  */

    PhotoInDB_Top = (struct photo *)malloc(sizeof(struct photo));
    sscanf(PhotoIdString,"%d",&PhotoInDB_Top->ph_ID);

    /*  add node to list  */

    PhotoInDB_Top->ph_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddPhotoInDB:    add to top\n");
#endif

    /*  build node  */

    PhotoInDB_New = (struct photo *)malloc(sizeof(struct photo));
    sscanf(PhotoIdString,"%d",&PhotoInDB_New->ph_ID);

    /*  add node to list  */

    PhotoInDB_New->ph_Next = PhotoInDB_Top;
    PhotoInDB_Top = PhotoInDB_New;

  }





#ifdef DEBUG
  printf("==>  AddPhotoInDB:  end exit\n");
#endif
  return(PhotoInDB_Top);
}
