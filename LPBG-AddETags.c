/*
  Adds ExifTags info to the list
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


struct petags *AddETags(struct petags *ETags_Top, struct petags *ETags_Temp)
{

  struct petags *ETags_New;

#ifdef DEBUG
  printf("==>  AddETags:  entry %d (%d)\n",ETags_Temp->pt_PID, ETags_Temp->pt_ID);
#endif

  if ( ETags_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddETags:    add first exiftag\n");
#endif

    /*  build node  */

    ETags_Top = (struct petags *)malloc(sizeof(struct petags));
    ETags_Top->pt_PID = ETags_Temp->pt_PID;
    ETags_Top->pt_ID = ETags_Temp->pt_ID;
    strcpy(ETags_Top->pt_Value, ETags_Temp->pt_Value);
    strcpy(ETags_Top->pt_DisplayValue, ETags_Temp->pt_DisplayValue);

    /*  add node to list  */

    ETags_Top->pt_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddETags:    add to top\n");
#endif

    /*  build node  */

    ETags_New = (struct petags *)malloc(sizeof(struct petags));
    ETags_New->pt_PID = ETags_Temp->pt_PID;
    ETags_New->pt_ID = ETags_Temp->pt_ID;
    strcpy(ETags_New->pt_Value, ETags_Temp->pt_Value);
    strcpy(ETags_New->pt_DisplayValue, ETags_Temp->pt_DisplayValue);

    /*  add node to list  */

    ETags_New->pt_Next = ETags_Top;
    ETags_Top = ETags_New;

  }





#ifdef DEBUG
  printf("==>  AddETags:  end exit\n");
#endif
  return(ETags_Top);
}
