/*
  processes the PhotoFlags elements
*/

/*
#undef DEBUG
#define DEBUG
*/

/*
#undef SHOWFOUND
#define SHOWFOUND
*/

/*
#undef MEMDEBUG
#define MEMDEBUG
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


struct pflags *ProcPF(struct pflags *PF_Top, char *srcString, int Id)
{

  int i;
  struct pflags *PF_Temp;

  char *strPFfalse;
  char *tmpString;

#ifdef DEBUG
  printf("==>  ProcPF:  entry %s, %d\n",srcString, Id);
#endif

  tmpString=(char *)malloc(strlen(srcString));

  strPFfalse=(char *)malloc( (sizeof(struct pflags) + 1) );
  for ( i = 0 ; i < sizeof(struct pflags) ; i++ )
    strPFfalse[i] = 0;
#ifdef MEMDEBUG
  printf("==>  ProcPF:  %d %d\n",i,sizeof(struct pflags));
#endif
  strPFfalse[i]='\0';

  /*
    need to parse the PhotoFlag entries
    create a new PF list entry
    set the appropriate values to TRUE
  */

  PF_Temp = (struct pflags *)malloc(sizeof(struct pflags));

  /*  set everything to FALSE  */

  memcpy(PF_Temp, strPFfalse, sizeof(struct pflags));

  /*  parse the values of the PhotoFlags  */

  PF_Temp->pf_PID = Id;

#ifdef DEBUG
  printf("==>  ProcPF:  starting to parse flags...\n");
#endif

  while ( srcString[0] != '\0' ) {
    strcpy(tmpString,NextTokenTrim(srcString));

#ifdef SHOWFOUND
    printf("==>  ProcPF:    Found:  %s\n",tmpString);
#endif


    if ( strcmp(tmpString,"HasTitle") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcPF:      HasTitle become TRUE\n");
#endif
      PF_Temp->pf_HasTitle = TRUE;
    }  /*  found a HasTitle PhotoFlags token  */

    if ( strcmp(tmpString,"HasCaption") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcPF:      HasCaption become TRUE\n");
#endif
      PF_Temp->pf_HasCaption = TRUE;
    }  /*  found a HasCaption PhotoFlags token  */

    if ( strcmp(tmpString,"HasKeywords") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcPF:      HasKeywords become TRUE\n");
#endif
      PF_Temp->pf_HasKeywords = TRUE;
    }  /*  found a HasKeywords PhotoFlags token  */

    if ( strcmp(tmpString,"HasCategories") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcPF:      HasCategories become TRUE\n");
#endif
      PF_Temp->pf_HasCategories = TRUE;
    }  /*  found a HasCategories PhotoFlags token  */

    if ( strcmp(tmpString,"HasExif") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcPF:      HasExif become TRUE\n");
#endif
      PF_Temp->pf_HasExif = TRUE;
    }  /*  found a HasExif PhotoFlags token  */

    if ( strcmp(tmpString,"HasComments") == 0 ) {
#ifdef DEBUG
      printf("==>  ProcPF:      HasComments become TRUE\n");
#endif
      PF_Temp->pf_HasComments = TRUE;
    }  /*  found a HasComments PhotoFlags token  */


  }  /*  while parsing the srcString  */

  /*  add to top of PF list  */

  PF_Temp->pf_Next = PF_Top;
  PF_Top = PF_Temp;


  /*  clean up and go home  */

  free(strPFfalse);
  free(tmpString);

#ifdef DEBUG
  printf("==>  ProcPF:  end exit\n");
#endif

  return(PF_Top);

}

