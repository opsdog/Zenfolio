/*
  Adds a gallery (PhotoSet) to the list
*/

/*
#undef DEBUG
#define DEBUG
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


#include "Zenfolio.h"

#include "LoadGroups.h"


#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif


struct gallery *AddGal(struct gallery *Gal_Top, struct gallery *Gal_Temp)
{

  struct gallery *Gal_Current;

#ifdef DEBUG
  printf("==>  AddGal:  entry %d %s\n",
	 Gal_Temp->gal_ID, Gal_Temp->gal_Title);
#endif

  if ( Gal_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddGal:    add first gallery\n");
#endif

    /*  build node  */

    Gal_Top = (struct gallery *)malloc(sizeof(struct gallery));
    Gal_Top->gal_ID = Gal_Temp->gal_ID;
    strcpy(Gal_Top->gal_Title,Gal_Temp->gal_Title);

    Gal_Top->gal_GroupIndex = Gal_Temp->gal_GroupIndex;
    Gal_Top->gal_AD = Gal_Temp->gal_AD;
    strcpy(Gal_Top->gal_Owner,Gal_Temp->gal_Owner);
    strcpy(Gal_Top->gal_Caption,Gal_Temp->gal_Caption);
    strcpy(Gal_Top->gal_CreatedOn,Gal_Temp->gal_CreatedOn);
    strcpy(Gal_Top->gal_ModifiedOn,Gal_Temp->gal_ModifiedOn);
    Gal_Top->gal_PhotoCount = Gal_Temp->gal_PhotoCount;
    Gal_Top->gal_PhotoBytes = Gal_Temp->gal_PhotoBytes;
    Gal_Top->gal_Views = Gal_Temp->gal_Views;
    strcpy(Gal_Top->gal_Type,Gal_Temp->gal_Type);
    Gal_Top->gal_TitlePhoto = Gal_Temp->gal_TitlePhoto;
    strcpy(Gal_Top->gal_UploadUrl,Gal_Temp->gal_UploadUrl);

    /*  add node to list  */

    Gal_Top->gal_Next = NULL;
  }
  else {
#ifdef DEBUG
    printf("==>  AddGal:    add to top\n");
#endif

    /*  build node  */

    Gal_Current = (struct gallery *)malloc(sizeof(struct gallery));
    Gal_Current->gal_ID = Gal_Temp->gal_ID;
    strcpy(Gal_Current->gal_Title,Gal_Temp->gal_Title);

    Gal_Current->gal_GroupIndex = Gal_Temp->gal_GroupIndex;
    Gal_Current->gal_AD = Gal_Temp->gal_AD;
    strcpy(Gal_Current->gal_Owner,Gal_Temp->gal_Owner);
    strcpy(Gal_Current->gal_Caption,Gal_Temp->gal_Caption);
    strcpy(Gal_Current->gal_CreatedOn,Gal_Temp->gal_CreatedOn);
    strcpy(Gal_Current->gal_ModifiedOn,Gal_Temp->gal_ModifiedOn);
    Gal_Current->gal_PhotoCount = Gal_Temp->gal_PhotoCount;
    Gal_Current->gal_PhotoBytes = Gal_Temp->gal_PhotoBytes;
    Gal_Current->gal_Views = Gal_Temp->gal_Views;
    strcpy(Gal_Current->gal_Type,Gal_Temp->gal_Type);
    Gal_Current->gal_TitlePhoto = Gal_Temp->gal_TitlePhoto;
    strcpy(Gal_Current->gal_UploadUrl,Gal_Temp->gal_UploadUrl);

    /*  add node to list  */

    Gal_Current->gal_Next = Gal_Top;
    Gal_Top = Gal_Current;
  }

#ifdef DEBUG
  printf("==>  AddGal:  end exit\n");
#endif

  return(Gal_Top);

}
