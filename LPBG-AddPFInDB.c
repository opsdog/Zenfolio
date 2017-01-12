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


struct pflags *AddPFInDB(struct pflags *PFInDb_Top, char *PhotoIdString)
{

  struct pflags *PFInDb_New;

#ifdef DEBUG
  printf("==>  AddPFInDb:  entry %s\n",PhotoIdString);
#endif

  if ( PFInDb_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddPFInDb:    add first PhotoId\n");
#endif

    /*  build node  */

    PFInDb_Top = (struct pflags *)malloc(sizeof(struct pflags));
    sscanf(PhotoIdString,"%d",&PFInDb_Top->pf_PID);

    /*  add node to list  */

    PFInDb_Top->pf_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddPFInDb:    add to top\n");
#endif

    /*  build node  */

    PFInDb_New = (struct pflags *)malloc(sizeof(struct pflags));
    sscanf(PhotoIdString,"%d",&PFInDb_New->pf_PID);

    /*  add node to list  */

    PFInDb_New->pf_Next = PFInDb_Top;
    PFInDb_Top = PFInDb_New;

  }





#ifdef DEBUG
  printf("==>  AddPFInDb:  end exit\n");
#endif
  return(PFInDb_Top);
}
