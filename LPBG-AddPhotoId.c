/*
  Adds *just* PhotoId to list
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


struct photo *AddPhotoId(struct photo *Photo_Top, struct photo *Photo_Temp)
{

  struct photo *Photo_New;

#ifdef DEBUG
  printf("==>  AddPhotoId:  entry %d\n",Photo_Temp->ph_ID);
#endif

  if ( Photo_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddPhotoId:    add first photo\n");
#endif

    /*  build node  */

    Photo_Top = (struct photo *)malloc(sizeof(struct photo));
    Photo_Top->ph_ID = Photo_Temp->ph_ID;

    /*  add node to list  */

    Photo_Top->ph_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddPhotoId:    add to top\n");
#endif

    /*  build node  */

    Photo_New = (struct photo *)malloc(sizeof(struct photo));
    Photo_New->ph_ID = Photo_Temp->ph_ID;

    /*  add node to list  */

    Photo_New->ph_Next = Photo_Top;
    Photo_Top = Photo_New;

  }





#ifdef DEBUG
  printf("==>  AddPhotoId:  end exit\n");
#endif
  return(Photo_Top);
}
