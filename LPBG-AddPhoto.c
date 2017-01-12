/*
  Adds photo info to the list
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


struct photo *AddPhoto(struct photo *Photo_Top, struct photo *Photo_Temp)
{

  struct photo *Photo_New;

#ifdef DEBUG
  printf("==>  AddPhoto:  entry %d\n",Photo_Temp->ph_ID);
#endif

  if ( Photo_Top == NULL ) {
#ifdef DEBUG
    printf("==>  AddPhoto:    add first photo\n");
#endif

    /*  build node  */

    Photo_Top = (struct photo *)malloc(sizeof(struct photo));
    Photo_Top->ph_ID = Photo_Temp->ph_ID;
    Photo_Top->ph_Width = Photo_Temp->ph_Width;
    Photo_Top->ph_Height = Photo_Temp->ph_Height;
    strcpy(Photo_Top->ph_Sequence, Photo_Temp->ph_Sequence);
    Photo_Top->ph_AD = Photo_Temp->ph_AD;
    strcpy(Photo_Top->ph_Owner, Photo_Temp->ph_Owner);
    strcpy(Photo_Top->ph_Title, Photo_Temp->ph_Title);
    strcpy(Photo_Top->ph_MimeType, Photo_Temp->ph_MimeType);
    Photo_Top->ph_Views = Photo_Temp->ph_Views;
    Photo_Top->ph_Size = Photo_Temp->ph_Size;
    Photo_Top->ph_Gallery = Photo_Temp->ph_Gallery;
    strcpy(Photo_Top->ph_OriginalUrl, Photo_Temp->ph_OriginalUrl);
    strcpy(Photo_Top->ph_UrlCore, Photo_Temp->ph_UrlCore);
    strcpy(Photo_Top->ph_UrlHost, Photo_Temp->ph_UrlHost);
    strcpy(Photo_Top->ph_UrlToken, Photo_Temp->ph_UrlToken);
    strcpy(Photo_Top->ph_PageUrl, Photo_Temp->ph_PageUrl);
    strcpy(Photo_Top->ph_MailboxId, Photo_Temp->ph_MailboxId);
    Photo_Top->ph_TextCn = Photo_Temp->ph_TextCn;
    Photo_Top->ph_PhotoListCn = Photo_Temp->ph_PhotoListCn;
    strcpy(Photo_Top->ph_Caption, Photo_Temp->ph_Caption);
    strcpy(Photo_Top->ph_FileName, Photo_Temp->ph_FileName);
    strcpy(Photo_Top->ph_UploadedOn, Photo_Temp->ph_UploadedOn);
    strcpy(Photo_Top->ph_TakenOn, Photo_Temp->ph_TakenOn);
    strcpy(Photo_Top->ph_Copyright, Photo_Temp->ph_Copyright);
    strcpy(Photo_Top->ph_Rotation, Photo_Temp->ph_Rotation);
    strcpy(Photo_Top->ph_ShortExif, Photo_Temp->ph_ShortExif);
    strcpy(Photo_Top->ph_IsVideo, Photo_Temp->ph_IsVideo);
    Photo_Top->ph_PricingKey = Photo_Temp->ph_PricingKey;

    /*  add node to list  */

    Photo_Top->ph_Next = NULL;

  } 
  else {
#ifdef DEBUG
    printf("==>  AddPhoto:    add to top\n");
#endif

    /*  build node  */

    Photo_New = (struct photo *)malloc(sizeof(struct photo));
    Photo_New->ph_ID = Photo_Temp->ph_ID;
    Photo_New->ph_Width = Photo_Temp->ph_Width;
    Photo_New->ph_Height = Photo_Temp->ph_Height;
    strcpy(Photo_New->ph_Sequence, Photo_Temp->ph_Sequence);
    Photo_New->ph_AD = Photo_Temp->ph_AD;
    strcpy(Photo_New->ph_Owner, Photo_Temp->ph_Owner);
    strcpy(Photo_New->ph_Title, Photo_Temp->ph_Title);
    strcpy(Photo_New->ph_MimeType, Photo_Temp->ph_MimeType);
    Photo_New->ph_Views = Photo_Temp->ph_Views;
    Photo_New->ph_Size = Photo_Temp->ph_Size;
    Photo_New->ph_Gallery = Photo_Temp->ph_Gallery;
    strcpy(Photo_New->ph_OriginalUrl, Photo_Temp->ph_OriginalUrl);
    strcpy(Photo_New->ph_UrlCore, Photo_Temp->ph_UrlCore);
    strcpy(Photo_New->ph_UrlHost, Photo_Temp->ph_UrlHost);
    strcpy(Photo_New->ph_UrlToken, Photo_Temp->ph_UrlToken);
    strcpy(Photo_New->ph_PageUrl, Photo_Temp->ph_PageUrl);
    strcpy(Photo_New->ph_MailboxId, Photo_Temp->ph_MailboxId);
    Photo_New->ph_TextCn = Photo_Temp->ph_TextCn;
    Photo_New->ph_PhotoListCn = Photo_Temp->ph_PhotoListCn;
    strcpy(Photo_New->ph_Caption, Photo_Temp->ph_Caption);
    strcpy(Photo_New->ph_FileName, Photo_Temp->ph_FileName);
    strcpy(Photo_New->ph_UploadedOn, Photo_Temp->ph_UploadedOn);
    strcpy(Photo_New->ph_TakenOn, Photo_Temp->ph_TakenOn);
    strcpy(Photo_New->ph_Copyright, Photo_Temp->ph_Copyright);
    strcpy(Photo_New->ph_Rotation, Photo_Temp->ph_Rotation);
    strcpy(Photo_New->ph_ShortExif, Photo_Temp->ph_ShortExif);
    strcpy(Photo_New->ph_IsVideo, Photo_Temp->ph_IsVideo);
    Photo_New->ph_PricingKey = Photo_Temp->ph_PricingKey;

    /*  add node to list  */

    Photo_New->ph_Next = Photo_Top;
    Photo_Top = Photo_New;

  }





#ifdef DEBUG
  printf("==>  AddPhoto:  end exit\n");
#endif
  return(Photo_Top);
}
