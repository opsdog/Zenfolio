/*
  if AD_Top = NULL
    this is the first node - add as AD_Top
  else if not dup
    add sorted
  else if dup
    call out
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


struct accdes *AddAD(struct accdes *AD_Top, struct accdes *AD_Temp)
{

  int doneP = FALSE;
  struct accdes *AD_Current = NULL;
  struct accdes *AD_OneBefore = NULL;
  struct accdes *AD_New = NULL;

#ifdef DEBUG
  printf("==>  AddAD:  entry %d\n",AD_Temp->ad_RealmId);
#endif

  if ( AD_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddAD:    Adding first node - exiting\n");
#endif

    /*  build the new node  */

    AD_Top = (struct accdes *)malloc(sizeof(struct accdes));
    AD_Top->ad_RealmId = AD_Temp->ad_RealmId;
    strcpy(AD_Top->ad_AccessType,AD_Temp->ad_AccessType);
    AD_Top->ad_IsDerived = AD_Temp->ad_IsDerived;
    strcpy(AD_Top->ad_PasswordHint,AD_Temp->ad_PasswordHint);
    strcpy(AD_Top->ad_SrcPasswordHint,AD_Temp->ad_SrcPasswordHint);

    /*  insert the new node  */

    AD_Top->ad_Next = NULL;

    return(AD_Top);
  }  /*  adding first node  */

#ifdef DEBUG
  printf("==>  AddAD:    not first entry\n");
#endif
 
  /*
    main loop if not first entry
  */

  AD_Current = AD_Top;
  AD_OneBefore = NULL;

  while ( AD_Current != NULL ) {

#ifdef DEBUG
    printf("==>  AddAD:    Top of while:  %d %d\n",
	   AD_Current->ad_RealmId,
	   AD_Temp->ad_RealmId);
#endif

    /****************************************************************/
    /*  easy one first - is RealmId aready accounted for?  */
    /****************************************************************/

    if ( AD_Current->ad_RealmId == AD_Temp->ad_RealmId ) {
#ifdef DEBUG
      printf("==>  AddAD:    RealmID already in the list - exiting\n");
#endif

      return(AD_Top);
    }  /*  RealmID already in the list in AD_Current position  */

    /****************************************************************/
    /*  check for placement within existing list and add  */
    /****************************************************************/

    if ( AD_Current->ad_RealmId > AD_Temp->ad_RealmId ) {
#ifdef DEBUG
      printf("==>  AddAD:    Mid insertion %d > %d\n",
	     AD_Current->ad_RealmId,
	     AD_Temp->ad_RealmId);
#endif

      /*  this one bit me in the ass  */

      if ( AD_Current->ad_Next != NULL )
	if ( AD_Current->ad_Next->ad_RealmId == AD_Temp->ad_RealmId ) {
#ifdef DEBUG
	  printf("==>  AddAD:      RealmID next in the list - exiting\n");
#endif

	  return(AD_Top);
	}  /*  RealmID already in the list in NEXT position  */

      /****************************************************************/
      /*  this condition does not represent a special case  */
      /****************************************************************/

#ifdef DEBUG
      if ( AD_Current == NULL ) printf("==>  AddAD:      NULL AD_Current\n");
#endif

      /****************************************************************/
      /*  add to top, but not first node  */
      /****************************************************************/

      if ( AD_OneBefore == NULL ) {
#ifdef DEBUG
	printf("==>  AddAD:      top add\n");
#endif

	/*  build the new node  */

	AD_New = (struct accdes *)malloc(sizeof(struct accdes));
	AD_New->ad_RealmId = AD_Temp->ad_RealmId;
	strcpy(AD_New->ad_AccessType,AD_Temp->ad_AccessType);
	AD_New->ad_IsDerived = AD_Temp->ad_IsDerived;
	strcpy(AD_New->ad_PasswordHint,AD_Temp->ad_PasswordHint);
	strcpy(AD_New->ad_SrcPasswordHint,AD_Temp->ad_SrcPasswordHint);

	/*  insert the new node  */

	AD_New->ad_Next = AD_Top;
	AD_Top = AD_New;
	
	return(AD_Top);
      }  /*  mid insertion top add  */

      /****************************************************************/
      /*  add to bottom  */
      /****************************************************************/

      if ( AD_Current->ad_Next == NULL ) {
#ifdef DEBUG
	printf("==>  AddAD:      bottom add\n");
#endif

	/*  build the new node  */

	AD_New = (struct accdes *)malloc(sizeof(struct accdes));
	AD_New->ad_RealmId = AD_Temp->ad_RealmId;
	strcpy(AD_New->ad_AccessType,AD_Temp->ad_AccessType);
	AD_New->ad_IsDerived = AD_Temp->ad_IsDerived;
	strcpy(AD_New->ad_PasswordHint,AD_Temp->ad_PasswordHint);
	strcpy(AD_New->ad_SrcPasswordHint,AD_Temp->ad_SrcPasswordHint);

	/*  insert the new node  */

	AD_New->ad_Next = NULL;
	AD_Current->ad_Next = AD_New;

#ifdef DEBUG
	printf("==>  AddAD:        AD_Current->ad_Next %d\n",AD_Current->ad_Next);
#endif

	return(AD_Top);

      }  /*  mid insertion bottom add */

      /****************************************************************/
      /*  this condition does not represent a special case  */
      /****************************************************************/

#ifdef DEBUG
      if ( AD_OneBefore->ad_Next == NULL ) printf("==>  AddAD:      NULL AD_OneBefore->ad_Next\n");
#endif


      /****************************************************************/
      /*  no special cases apply - just a normal mid insertion  */
      /****************************************************************/


#ifdef DEBUG
      printf("==>  AddAD:      OneBefore %d\n",AD_OneBefore->ad_RealmId);
      printf("==>  AddAD:      Current   %d\n",AD_Current->ad_RealmId);
#endif

      /*  build the new node */

      AD_New = (struct accdes *)malloc(sizeof(struct accdes));
      AD_New->ad_RealmId = AD_Temp->ad_RealmId;
      strcpy(AD_New->ad_AccessType,AD_Temp->ad_AccessType);
      AD_New->ad_IsDerived = AD_Temp->ad_IsDerived;
      strcpy(AD_New->ad_PasswordHint,AD_Temp->ad_PasswordHint);
      strcpy(AD_New->ad_SrcPasswordHint,AD_Temp->ad_SrcPasswordHint);

      /*  insert the new node */

      AD_New->ad_Next = AD_Current;
      AD_OneBefore->ad_Next = AD_New;

      return(AD_Top);

    }  /*  found mid-insertion point  */

    AD_OneBefore = AD_Current;
    AD_Current = AD_Current->ad_Next;
  }  /*  while checking current list  */

#ifdef DEBUG
  printf("==>  AddAD:    out of while %d\n",AD_OneBefore->ad_RealmId);
#endif

  if ( doneP ) {
    printf("==>  AddAD:    TRUE\n");
  }
  else {

    /****************************************************************/
    /*  out of while without adding - add to bottom  */
    /****************************************************************/

#ifdef DEBUG
    printf("==>  AddAD:    FALSE\n");
#endif

    /*  build the new node  */

    AD_Current = (struct accdes *)malloc(sizeof(struct accdes));
    AD_Current->ad_RealmId = AD_Temp->ad_RealmId;
    strcpy(AD_Current->ad_AccessType,AD_Temp->ad_AccessType);
    AD_Current->ad_IsDerived = AD_Temp->ad_IsDerived;
    strcpy(AD_Current->ad_PasswordHint,AD_Temp->ad_PasswordHint);
    strcpy(AD_Current->ad_SrcPasswordHint,AD_Temp->ad_SrcPasswordHint);

    /*  insert the new node  */

    AD_Current->ad_Next = NULL;
    AD_OneBefore->ad_Next = AD_Current;
  }

#ifdef DEBUG
  printf("==>  AddAD:  end exit\n");
#endif
  return(AD_Top);
}
