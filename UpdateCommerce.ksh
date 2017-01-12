#!/bin/ksh
#!/usr/local/bin/ksh
#
#  script to pull the photo ids from zenfolio and update the
#  commerce table in the racepics database.
#

##  turn on/off DBLoad profiling

DBProfile="Prof"  ##  turn on

#  initialize stuff

ProgPrefix=/Volumes/External300/DBProgs/RacePics
unset MYSQL
. /Volumes/External300/DBProgs/FSRServers/DougPerfData/SetDBCase.ksh

if [ -d /Volumes/External300/DBProgs/ZenFolio ]
then
  ProgPrefix=/Volumes/External300/DBProgs/ZenFolio
else
  ProgPrefix=/Users/douggreenwald/ZenFolio
fi

#  remove files from previous runs

rm -f AuthToken.xml AuthToken >/dev/null 2>&1
rm -f XML*.xml >/dev/null 2>&1
rm -f BringCurrent.sql >/dev/null 2>&1
rm -f /tmp/dougee* /tmp/Galleries.* >/dev/null 2>&1
rm -f /tmp/Photos.* >/dev/null 2>&1

#  drop tables that are about to be created (not updated)

$MYSQL <<EOF
use racepics;

drop table zengallery;

create table zengallery (
gallid   varchar(20) not null primary key,
gallname varchar(50) default 'NULL'
)
engine = MyISAM
${DBdirClause}
;

EOF

#  make sure required support files exist

echo "Building required support files..."

(cd ${ProgPrefix}; make >/dev/null 2>&1 )
(cd ${ProgPrefix}/../RacePics; make >/dev/null 2>&1 )

#  get auth token for zenfolio

echo "Getting authentication credentials..."

${ProgPrefix}/GetAuthToken | egrep 'Set-Cookie:' | awk '{ print $2 }' | awk -F\= '{ print $2 }' | awk -F\; '{ print $1 }' | read AuthToken
## chmod 0400 AuthToken.xml
## AuthToken=`${ProgPrefix}/ParseXMLFile AuthToken.xml | awk '{ print $2 }'`
echo ${AuthToken} > AuthToken
chmod 0400 AuthToken

#  dump the gallery IDs into a file

echo "Getting gallery IDs Loop..."
echo -n "  "
Ret=1
while (( $Ret == 1 ))
do
  echo -n "."
  ${ProgPrefix}/GetAllGalleriesXML ${AuthToken} 
  chmod 0600 XML-AllGalleries.xml
  ${ProgPrefix}/ParseAllGalleries XML-AllGalleries.xml > /tmp/Galleries.$$ 2>/dev/null
  Ret=$?
done
echo


#  build the zengallery table

rm -f /tmp/ZenGallery-$$.sql >/dev/null 2>&1
touch /tmp/ZenGallery-$$.sql

exec 4</tmp/Galleries.$$
while read -u4 gallid gallname
do
  Res=`echo $gallname | sed s/\"// | sed s/\"//`
  echo "insert into zengallery (gallid, gallname) values ('${gallid}', '${Res}');" >> /tmp/ZenGallery-$$.sql
done
exec 4<&-

${MYSQL}  <<EOF
use racepics;
source /tmp/ZenGallery-$$.sql
EOF

rm -f /tmp/ZenGallery-$$.sql >/dev/null 2>&1

##
##  dump photo info into a file by cycling through each gallery
##

##  uncomment this for testing...

##  cat /tmp/Galleries.$$ | head -5 > /tmp/GallReduce.$$
##  mv /tmp/GallReduce.$$ /tmp/Galleries.$$

##  on with normal processing

echo "Getting photo information from galleries loop..."

touch /tmp/Photos.$$
exec 4</tmp/Galleries.$$
while read -u4 GalleryID Title
do
  echo -n "  Gallery: ${Title}  "
  Ret=1
  while (( $Ret == 1 ))
  do
    echo -n "."
    rm -f /tmp/dougee* XML-PhotoSet.xml >/dev/null 2>&1
    ${ProgPrefix}/GetPhotoSetXML ${AuthToken} ${GalleryID}
    chmod 0600 XML-PhotoSet.xml
    ##  echo "${ProgPrefix}/ParsePhotoSet XML-PhotoSet.xml >> /tmp/Photos.$$"
    ${ProgPrefix}/ParsePhotoSet XML-PhotoSet.xml >> /tmp/Photos.$$ 2>/dev/null
    Ret=$?
  done  ##  while pulling and parsing from zenfolio
  echo
done  #  while reading each gallery id
exec 4<&-

#  determine if image is already in the database.
#  if in database, determine if zfid is valid
#    if zfid missing or changed, update
#  if not in database, insert

NumPhotos=`wc -l /tmp/Photos.$$ | awk '{ print $1 }'`

echo "Creating inserts and updates for $NumPhotos images..."

exec 4</tmp/Photos.$$

while read -u4 Gallery ZFID Filename
do

#  echo "  ${ZFID} ${Filename}"
  Basename=`echo ${Filename} | awk -F. '{ print $1 }'`
  Result=`${ProgPrefix}/../RacePics/NewQuery racepics "select id from commerce where id = '${Basename}'"`
  
  #  is the image in the database?

  if [ -z "${Result}" ]
  then

    #  image not in database - generate an insert

    echo "insert into commerce (id, zfgal, zfid) values ('${Basename}', '${Gallery}', '${ZFID}');" >> BringCurrent.sql

  else

    echo "update commerce set zfgal='${Gallery}', zfid='${ZFID}' where id = '${Basename}';" >> BringCurrent.sql

  fi  #  if image is not in database

done
exec 4<&-


##
##  update the database
##

echo "Updating the database..."

Count=`${ProgPrefix}/../RacePics/NewQuery racepics "select count(*) from commerce`
echo -n "  $Count --> "

exec 4<BringCurrent.sql
while read -u4 Junk
do
  ${MYSQL}  <<EOF
  use racepics;
  $Junk
EOF
done > DuplicateKeys.txt 2>&1
exec 4<&-

Count=`${ProgPrefix}/../RacePics/NewQuery racepics "select count(*) from commerce`
echo "$Count"

##
##  add the vehnum to commerce
##

echo "Adding vehicle numbers to commerce..."

Count=`${ProgPrefix}/../RacePics/NewQuery racepics "select count(*) from commerce where vehnum != 'NULL'`
echo -n "  $Count --> "

rm -f dougee.1 AddVehNum.sql >/dev/null 2>&1
touch dougee.1 AddVehNum.sql

${ProgPrefix}/../RacePics/NewQuery racepics "select id, vehnum from image where vehnum != 'NULL' order by id" > dougee.1

exec 4<dougee.1
while read -u4 ImageID VehNum
do
  echo "update commerce set vehnum = '${VehNum}' where id like '${ImageID}%';" >> AddVehNum.sql
done
exec 4<&-

rm -f dougee.1 >/dev/null 2>&1

${MYSQL}  <<EOF
use racepics;
source ./AddVehNum.sql;
EOF

Count=`${ProgPrefix}/../RacePics/NewQuery racepics "select count(*) from commerce where vehnum != 'NULL'`
echo "$Count"

##
##  report errors
##

NumDups=`wc -l DuplicateKeys.txt | awk '{ print $1 }'`

if [ $NumDups != 0 ]
then
  echo "$NumDups duplicate keys for table commerce"
fi
