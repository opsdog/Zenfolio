#!/bin/ksh
##
##  script to read the DuplicateKeys.txt file and show where image
##  duplications exist
##

#  initialize stuff

if [ -d /Volumes/External300/DBProgs/RacePics ]
then
  ProgPrefix=/Volumes/External300/DBProgs/RacePics
else
  ProgPrefix=/Users/douggreenwald/RacePics
fi

DBLocation=`../RacePics/FindDB.ksh`
if [ "$DBLocation" = "notfound" ]
then
  echo "no database - bailing..."
  exit
fi
if [ "$DBLocation" = "localhost" ]
then
  MYSQL="mysql -u root"
else
  MYSQL="mysql -h big-mac -u doug -pILikeSex"
fi

##
##  process the duplicate primary keys
##

for Image in `cat DuplicateKeys.txt | awk '{ print $9 }' | tr -d \'`
do

  ##  find out what gallery this exists in

  GalleryString=`${ProgPrefix}/NewQuery racepics "select commerce.id, zengallery.gallname, zengallery.gallid from commerce, zengallery where commerce.id = '$Image' and zengallery.gallid = commerce.zfgal"`
  echo $GalleryString | read Gallery GallName GallID

  ##  find out what gallery it is being duplicated in

  DupGallID=`grep "'$Image'" BringCurrent.sql | tail -1 | awk '{ print $9 }' | awk -F\' '{ print $2 }'`
  DupGallName=`${ProgPrefix}/NewQuery racepics "select gallname from zengallery where gallid = '${DupGallID}'"`

  echo "$Image exists in $GallName duplicated in $DupGallName"

done
