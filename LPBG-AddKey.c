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


struct pkeys *AddKey(struct pkeys *Key_Top, struct pkeys *Key_Temp)
{

  struct pkeys *Key_New;

#ifdef DEBUG
  printf("==>  AddKey:  entry %d\n",Key_Temp->pk_PID);
#endif

  if ( Key_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddKey:    add first keyword\n");
#endif

    /*  build node  */

    Key_Top = (struct pkeys *)malloc(sizeof(struct pkeys));
    Key_Top->pk_PID = Key_Temp->pk_PID;
    strcpy(Key_Top->pk_Keyword, Key_Temp->pk_Keyword);

    /*  add node to list  */

    Key_Top->pk_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddKey:    add to top\n");
#endif

    /*  build node  */

    Key_New = (struct pkeys *)malloc(sizeof(struct pkeys));
    Key_New->pk_PID = Key_Temp->pk_PID;
    strcpy(Key_New->pk_Keyword, Key_Temp->pk_Keyword);

    /*  add node to list  */

    Key_New->pk_Next = Key_Top;
    Key_Top = Key_New;

  }





#ifdef DEBUG
  printf("==>  AddKey:  end exit\n");
#endif
  return(Key_Top);
}
