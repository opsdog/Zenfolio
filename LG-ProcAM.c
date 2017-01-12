/*
  function to process an access mask - parse and add to list
*/

/*
#undef DEBUG
#define DEBUG
*/

/*
#undef SHOWFOUND
#define SHOWFOUND
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


struct accmask *ProcAM(struct accmask *AM_Top, char *srcString, char *srcType, int Id)
{

  int i;
  struct accmask *AM_Temp;

  char *strAMfalse;
  char *tmpString;

#ifdef DEBUG
  printf("==>  ProcAM:  entry %s, %s, %d\n",srcString, srcType, Id);
#endif

  tmpString=(char *)malloc(strlen(srcString));

  strAMfalse=(char *)malloc( (sizeof(struct accmask) + 1) );
  for ( i = 0 ; i < sizeof(struct accmask) ; i++ )
    strAMfalse[i] = 0;

#ifdef MEMDEBUG
  printf("DOUGEE:  %d %d\n",i,sizeof(struct accmask));
#endif

  strAMfalse[i]='\0';

  /*
    need to parse the AccessMask
    create a new AM list entry
    set the appropriate values to TRUE
  */

  AM_Temp = (struct accmask *)malloc(sizeof(struct accmask));

  /*  set everything to FALSE  */

  memcpy(AM_Temp, strAMfalse, sizeof(struct accmask));
  strcpy(AM_Temp->am_Type,srcType);

  if ( strcmp(srcType,"P") == 0 )
    AM_Temp->am_PID = Id;
  else
    AM_Temp->am_PID = 0;

  if ( strcmp(srcType,"PS") == 0 )
    AM_Temp->am_PID = Id;
  else
    AM_Temp->am_PSID = 0;

  if ( strcmp(srcType,"AD") == 0 )
    AM_Temp->am_ADID = Id;
  else
    AM_Temp->am_ADID = 0;

  AM_Temp->am_Next = NULL;

#ifdef DEBUG
  if (!AM_Temp->am_HideVisits)
    printf("==>  ProcAM:      HideVisits FALSE\n");
  if (!AM_Temp->am_ProtectMedium)
    printf("==>  ProcAM:      ProtectMedium FALSE\n");
#endif

  /*  parse the values of the AccessMask  */

  while ( srcString[0] != '\0' ) {
    strcpy(tmpString,NextTokenTrim(srcString));

#ifdef SHOWFOUND
    printf("==>  ProcAM:      Found:  %s\n",tmpString);
#endif

    if ( strcmp(tmpString,"HideDateCreated") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      HideDateCreated becomes TRUE\n");
#endif
      AM_Temp->am_HideDateCreated = TRUE;
    }  /*  found a HideDataCreated AccessMask token  */

    if ( strcmp(tmpString,"HideDateModified") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      HideDateModified becomes TRUE\n");
#endif
      AM_Temp->am_HideDateModified = TRUE;
    }  /*  found a HideDateModified AccessMask token */

    if ( strcmp(tmpString,"HideDateTaken") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      HideDateTaken becomes TRUE\n");
#endif
      AM_Temp->am_HideDateTaken = TRUE;
    }  /*  found a HideDateTaken AccessMask token */

    if ( strcmp(tmpString,"HideMetaData") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      HideMetaData becomes TRUE\n");
#endif
      AM_Temp->am_HideMetaData = TRUE;
    }  /*  found a HideMetaData AccessMask token */

    if ( strcmp(tmpString,"HideUserStats") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      HideUserStats becomes TRUE\n");
#endif
      AM_Temp->am_HideUserStats = TRUE;
    }  /*  found a HideUserStats AccessMask token */

    if ( strcmp(tmpString,"HideVisits") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      HideVisits becomes TRUE\n");
#endif
      AM_Temp->am_HideVisits = TRUE;
    }  /*  found a HideVisits AccessMask token */

    if ( strcmp(tmpString,"NoCollections") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoCollections becomes TRUE\n");
#endif
      AM_Temp->am_NoCollections = TRUE;
    }  /*  found a NoCollections AccessMask token */

    if ( strcmp(tmpString,"NoPrivateSearch") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoPrivateSearch becomes TRUE\n");
#endif
      AM_Temp->am_NoPrivateSearch = TRUE;
    }  /*  found a NoPrivateSearch AccessMask token */

    if ( strcmp(tmpString,"NoPublicSearch") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoPublicSearch becomes TRUE\n");
#endif
      AM_Temp->am_NoPublicSearch = TRUE;
    }  /*  found a NoPublicSearch AccessMask token */

    if ( strcmp(tmpString,"NoRecentList") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoRecentList becomes TRUE\n");
#endif
      AM_Temp->am_NoRecentList = TRUE;
    }  /*  found a NoRecentList AccessMask token */

    if ( strcmp(tmpString,"ProtectExif") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectExif becomes TRUE\n");
#endif
      AM_Temp->am_ProtectExif = TRUE;
    }  /*  found a ProtectExif AccessMask token */

    if ( strcmp(tmpString,"ProtectExtraLarge") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectExtraLarge becomes TRUE\n");
#endif
      AM_Temp->am_ProtectExtraLarge = TRUE;
    }  /*  found a ProtectExtraLarge AccessMask token  */

    if ( strcmp(tmpString,"ProtectLarge") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectLarge becomes TRUE\n");
#endif
      AM_Temp->am_ProtectLarge = TRUE;
    }  /*  found a ProtectLarge AccessMask token */

    if ( strcmp(tmpString,"ProtectMedium") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectMedium becomes TRUE\n");
#endif
      AM_Temp->am_ProtectMedium = TRUE;
    }  /*  found a ProtectMedium AccessMask token */

    if ( strcmp(tmpString,"ProtectOriginals") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectOriginals becomes TRUE\n");
#endif
      AM_Temp->am_ProtectOriginals = TRUE;
    }  /*  found a ProtectOriginals AccessMask token */

    if ( strcmp(tmpString,"ProtectXXLarge") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectXXLarge becomes TRUE\n");
#endif
      AM_Temp->am_ProtectXXLarge = TRUE;
    }  /*  found a ProtectXXLarge AccessMask token */

    if ( strcmp(tmpString,"ProtectGuestbook") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectGuestbook becomes TRUE\n");
#endif
      AM_Temp->am_ProtectGuestbook = TRUE;
    }  /*  found a ProtectGuestbook AccessMask token */

    if ( strcmp(tmpString,"NoPublicGBPosts") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoPublicGBPosts becomes TRUE\n");
#endif
      AM_Temp->am_NoPublicGBPosts = TRUE;
    }  /*  found a NoPublicGBPosts AccessMask token */

    if ( strcmp(tmpString,"NoPrivateGBPosts") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoPrivateGBPosts becomes TRUE\n");
#endif
      AM_Temp->am_NoPrivateGBPosts = TRUE;
    }  /*  found a NoPrivateGBPosts AccessMask token */

    if ( strcmp(tmpString,"NoAnonGBPosts") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoAnonGBPosts becomes TRUE\n");
#endif
      AM_Temp->am_NoAnonGBPosts = TRUE;
    }  /*  found a NoAnonGBPosts AccessMask token */

    if ( strcmp(tmpString,"ProtectComments") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectComments becomes TRUE\n");
#endif
      AM_Temp->am_ProtectComments = TRUE;
    }  /*  found a ProtectComments AccessMask token */

    if ( strcmp(tmpString,"NoPublicComments") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoPublicComments becomes TRUE\n");
#endif
      AM_Temp->am_NoPublicComments = TRUE;
    }  /*  found a NoPublicComments AccessMask token */

    if ( strcmp(tmpString,"NoPrivateComments") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoPrivateComments becomes TRUE\n");
#endif
      AM_Temp->am_NoPrivateComments = TRUE;
    }  /*  found a NoPrivateComments AccessMask token */

    if ( strcmp(tmpString,"NoAnonComments") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      NoAnonComments becomes TRUE\n");
#endif
      AM_Temp->am_NoAnonComments = TRUE;
    }  /*  found a NoAnonComments AccessMask token */

    if ( strcmp(tmpString,"PasswordProtOrig") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      PasswordProtOrig becomes TRUE\n");
#endif
      AM_Temp->am_PasswordProtOrig = TRUE;
    }  /*  found a PasswordProtOrig AccessMask token */

    if ( strcmp(tmpString,"ProtectAll") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcAM:      ProtectAll becomes TRUE\n");
#endif
      AM_Temp->am_ProtectAll = TRUE;
    }  /*  found a ProtectAll AccessMask token */


  }  /*  while parsing AccessMask tokens  */

  /*  add to top of AM list  */

  AM_Temp->am_Next = AM_Top;
  AM_Top = AM_Temp;

  free(tmpString);
  free(strAMfalse);

#ifdef DEBUG
  printf("==>  ProcAM:  end exit\n");
#endif

  return(AM_Top);
}
