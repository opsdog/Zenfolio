/*
  Adds category info to the list
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


struct pcats *AddCat(struct pcats *Cat_Top, struct pcats *Cat_Temp)
{

  struct pcats *Cat_New;

#ifdef DEBUG
  printf("==>  AddCat:  entry %d\n",Cat_Temp->pc_PID);
#endif

  if ( Cat_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddCat:    add first category\n");
#endif

    /*  build node  */

    Cat_Top = (struct pcats *)malloc(sizeof(struct pcats));
    Cat_Top->pc_PID = Cat_Temp->pc_PID;
    Cat_Top->pc_Category = Cat_Temp->pc_Category;

    /*  add node to list  */

    Cat_Top->pc_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddCat:    add to top\n");
#endif

    /*  build node  */

    Cat_New = (struct pcats *)malloc(sizeof(struct pcats));
    Cat_New->pc_PID = Cat_Temp->pc_PID;
    Cat_New->pc_Category = Cat_Temp->pc_Category;

    /*  add node to list  */

    Cat_New->pc_Next = Cat_Top;
    Cat_Top = Cat_New;

  }





#ifdef DEBUG
  printf("==>  AddCat:  end exit\n");
#endif
  return(Cat_Top);
}
