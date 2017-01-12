#!/bin/ksh
##
##  create the zenfolio database and tables
##
##  populates live from my zenfolio galleries
##

##  initialize stuff

MyEMail="doug.greenwald@gmail.com"

PATH="${PATH}:."

if [ -d /Volumes/External300/DBProgs/RacePics ]
then
  ProgPrefix="/Volumes/External300/DBProgs/RacePics"
else
  ProgPrefix="/Users/douggreenwald/RacePics"
fi

DBLocation=`${ProgPrefix}/FindDB.ksh`
if [ "$DBLocation" = "notfound" ]
then
  echo "no database - bailing..."
  exit
fi
if [ "$DBLocation" = "localhost" ]
then
  MYSQL="mysql -u root -A"
  DBdatadir=/Volumes/External300/mysqldata/
  DBindexdir=/Volumes/QPBackup/mysqldata/
else
  MYSQL="mysql -h big-mac -u doug -pILikeSex -A"
  DBdatadir=/Volumes/DBdata
  DBindexdir=/Volumes/DBindex
fi

WorkDir=`pwd`
InsertFile="${WorkDir}/ZenInserts.sql"
UpdateFile="${WorkDir}/ZenUpdates.sql"


echo
echo "Working from:"
echo "  $DBLocation"
echo "  $InsertFile"
echo "  $UpdateFile"
echo

##
##  re-create the zenfolio database and tables
##

$MYSQL <<EOF

drop database zenfolio;
create database zenfolio;

use zenfolio;

##
##  table to hold the PhotoSet information
##

create table PhotoSet (
PS_ID                    int not null,

#  Level 1 fields

PS_CreatedOn             date,
PS_ModifiedOn            date,
PS_Type                  enum ('Gallery', 'Collection'),
PS_FeaturedIndex         int,
P_ID                     int,     ##  foreign key into zenPhoto table
PS_PhotoCount            int,
PS_Views                 int,
PS_UploadURL             varchar(256),
PS_PageURL               varchar(256),
PS_MailboxID             varchar(256),
PS_TextCn                int,

# Level 2 fields

PS_Caption               varchar(1024),
#  keywords will be their own table
#  categories will be a pivot table
PS_IsRandomTitlePhoto    boolean,

#  Full level fields

# parent groups will be a pivot table
PS_PhotoBytes            int,

primary key (PS_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

create table Gallery (
GAL_ID               int not null,
GAL_GroupIndex       int,
GAL_Title            varchar(256),
GAL_AD               int,
GAL_Owner            varchar(50),
GAL_HideBranding     boolean default FALSE,
GAL_Caption          varchar(1024),
GAL_CreatedOn        date,
GAL_ModifiedOn       date,
GAL_PhotoCount       int,
GAL_PhotoBytes       int,
GAL_Views            int,
GAL_Type             enum ('Gallery', 'Collection'),
GAL_TitlePhoto       int,                  ##  FK to Photo
GAL_UploadUrl        varchar(256),

primary key (GAL_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  table to hold the Photo information
##

create table Photo (
P_ID                     int not null,

#  Level 1 fields

P_Width                  int,
P_Height                 int,
P_Sequence               varchar(100),
AD_ID                    int,   ##  foreign key to Access Descriptor table
P_Owner                  varchar(100),
P_Title                  varchar(256),
P_MimeType               varchar(100),
P_Views                  int default 0,
P_Size                   int,
P_Gallery                int,
P_OriginalURL            varchar(1024),
P_UrlCore                varchar(1024),
P_UrlHost                varchar(256),
P_UrlToken               varchar(512),
P_PageUrl                varchar(512),
P_MailboxID              varchar(256),
P_TextCn                 int,
P_PhotoListCn            int,
P_IsVideo                boolean default FALSE,
P_PricingKey             int,
# Flags will be a seperate table

#  Level 2 fields

P_Caption                varchar(2046),
P_FileName               varchar(256),
## P_UploadedOn             date,
## P_TakenOn                date,
P_UploadedOn             varchar(50),
P_TakenOn                varchar(50),
#  keywords will be their own table
#  categories will be a pivot table
P_Copyright              varchar(1024),
P_Rotation               enum ('None', 'Rotate90', 'Rotate180', 'Rotate270', 'Flip', 'Rotate90Flip', 'Rotate180Flip', 'Rotate270Flip'),
#  exif tags will be their own table
P_ShortExif              varchar(1024),

primary key (P_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  table to hold the EXIF data of a particular Photo
##

create table ExifTag (
P_ID                      int not null,  ## PK and foreign key to Photo table
EX_ID                     int not null,  ##  ExifTag ID
EX_Value                  varchar(256),  ##  the Exif value
EX_DisplayValue           varchar(256),  ##  the Exif value as displayed

primary key (P_ID, EX_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  table to hold the Flags associated with a particular Photo
##

create table PhotoFlags (
P_ID                     int not null,  ##  PK and foreign key to Photo table
PF_HasTitle              boolean default FALSE,
PF_HasCaption            boolean default FALSE,
PF_HasKeywords           boolean default FALSE,
PF_HasCategories         boolean default FALSE,
PF_HasExif               boolean default FALSE,
PF_HasComments           boolean default FALSE,

primary key (P_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  table to hold the site-wide Photo/PhotoSet Categories
##  this is the category list as defined by ZenFolio
##

create table CategoryInfo (
CAT_Code                 int not null,
CAT_DisplayName          varchar(100),
isTopLevelCategory       boolean default FALSE,
isSubCatDefinition       boolean default FALSE,

primary key (CAT_Code)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  the table of categories as assigned to each photo
##

create table PhotoCat (
P_ID                     int not null,    ##  PK and FK to Photo table
PC_Category              int not null,    ##  PK

primary key (P_ID, PC_Category)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  the table of keywords as assigned to each photo
##

create table Keywords (
P_ID                     int not null,    ##  PK and FK to Photo table
KW_Keyword               varchar(256),    ##  PK

primary key (P_ID, KW_Keyword)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  table to hold the AccessType
##

create table AccessType (
AT_Type                 enum ('Photo', 'PhotoSet', 'Group', 'Collection'),
AT_ID                   int not null,  ##  points to the object id
AT_Private              boolean default FALSE,
AT_Public               boolean default FALSE,
AT_UserList             boolean default FALSE,
AT_Password             boolean default FALSE,

primary key (AT_Type, AT_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##   table to hold the AccessMask
##

create table AccessMask (
AM_Type                 enum ('Photo', 'PhotoSet', 'Group', 'AD') not null,
P_ID                    int not null,  ##  PK and foreign key to Photo
PS_ID                   int not null,  ##  PK and foreign key to PhotoSet
AD_ID                   int not null,  ##  PK and FK to AccessDescriptor
AM_HideDateCreated      boolean default FALSE,
AM_HideDateModified     boolean default FALSE,
AM_HideDateTaken        boolean default FALSE,
AM_HideMetaData         boolean default FALSE,
AM_HideUserStats        boolean default FALSE,
AM_HideVisits           boolean default FALSE,
AM_NoCollections        boolean default FALSE,
AM_NoPrivateSearch      boolean default FALSE,
AM_NoPublicSearch       boolean default FALSE,
AM_NoRecentList         boolean default FALSE,
AM_ProtectExif          boolean default FALSE,
AM_ProtectExtraLarge    boolean default FALSE,
AM_ProtectLarge         boolean default FALSE,
AM_ProtectMedium        boolean default FALSE,
AM_ProtectOriginals     boolean default FALSE,
AM_ProtectGuestbook     boolean default FALSE,
AM_NoPublicGBPosts      boolean default FALSE,
AM_NoPrivateGBPosts     boolean default FALSE,
AM_NoAnonGBPosts        boolean default FALSE,
AM_ProtectComments      boolean default FALSE,
AM_NoPublicComments     boolean default FALSE,
AM_NoPrivateComments    boolean default FALSE,
AM_NoAnonComments       boolean default FALSE,
AM_PasswordProtOrig     boolean default FALSE,
AM_ProtectAll           boolean default FALSE,

primary key (AM_Type, P_ID, PS_ID, AD_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  AccessDescriptor table
##

create table AccessDescriptor (
AD_RealmId              int not null,    ##  RealmId
AD_IDControlled         int not null,    ##  FK to photoset, group, etc
AD_Type                 enum('Public', 'Private', 'Password', 'UserList'),
AD_AccessMask           int,             ##  FK to AccessMask
#  Viewers will be a pivot table - list of users iff type is UserList
AD_IsDerived            boolean default FALSE,
AD_PasswordHint         varchar(100),
AD_SrcPasswordHint      varchar(100),

primary key (AD_RealmId, AD_IDControlled)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  file table
##

create table File (
File_ID                  int not null,
File_Width               int,
File_Height              int,
File_Sequence            varchar(50),
File_MIMEType            varchar(25),
File_URLCore             varchar(50),
File_URLHost             varchar(50),

primary key (File_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  address table
##

create table Address (
ADR_ID                   int not null,
ADR_FirstName            varchar(50),
ADR_LastName             varchar(50),
ADR_CompanyName          varchar(100),
ADR_Street               varchar(100),
ADR_Street2              varchar(100),
ADR_City                 varchar(50),
ADR_ZIP                  varchar(20),
ADR_State                varchar(25),
ADR_Country              varchar(50),
ADR_Phone                varchar(25),
ADR_Phone2               varchar(25),
ADR_Fax                  varchar(25),
ADR_URL                  varchar(100),
ADR_Email                varchar(100),
ADR_Other                varchar(100),

primary key (ADR_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  PhotoSet Group table
##

create table psGroup (
GRP_ID                   int not null,

#  level 1 fields

GRP_CreatedOn            date,
GRP_ModifiedOn           date,
GRP_PageURL              varchar(75),
GRP_TitlePhone           int,          ##  FK to Photo
GRP_MailboxId            varchar(100),
GRP_ImmedChildrenCount   int,
GRP_TextCn               int,

#  level 2 fields

GRP_Caption              varchar(100),

#  full level fields

GRP_CollectionCount      int,
GRP_SubGroupCount        int,
GRP_GalleryCount         int,
GRP_PhotoCount           int,
#  ParentGroups will be a pivot table
#  GroupElements will be a seperate table

primary key (GRP_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  group element table - fetched by LoadGroupHierarchy
##

create table GroupElement (
GE_ID                     int not null,
GE_GroupIndex             int,
GE_Title                  varchar(100),
GE_AD                     int,  ##  FK to AccessDescriptor
GE_Owner                  varchar(100),
GE_HideBranding           boolean default FALSE,
GE_CreatedOn              date,
GE_ModifiedOn             date,

primary key (GE_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  message table
##

create table Message (
MSG_ID                    int not null,
MSG_MailboxId             varchar(50),
MSG_PostedOn              date,
MSG_PosterName            varchar(75),
MSG_PosterLoginName       varchar(50),
MSG_PosterURL             varchar(100),
MSG_PosterEmail           varchar(75),
MSG_Body                  blob(2048),
MSG_IsPrivate             boolean,

primary key (MSG_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  User table
##

create table zenUser (
USER_ID                  int not null,
USER_LoginName           varchar(20),
USER_DisplayName         varchar(50),
USER_FirstName           varchar(50),
USER_LastName            varchar(100),
USER_PrimaryEmail        varchar(75),
USER_BioPhoto            int,            ##  FK to File
USER_Bio                 blob(2048),
USER_Views               int,
USER_GalleryCount        int,
USER_CollectionCount     int,
USER_PhotoCount          int,
USER_PhotoBytes          float,
USER_UserSince           date,
USER_LastUpdated         date,
USER_PubAddress          int,            ##  FK to Address
USER_PerAddress          int,            ##  FK to Address
#  RecentPhotoSets will be a pivot table
#  FeaturedPhotoSets will be a pivot table
USER_RootGroup           int,            ##  FK to Group
USER_ReferralCode        varchar(25),
USER_ExpiresOn           date,
USER_Balance             float,
USER_DomainName          varchar(75),
USER_StorageQuota        float,
USER_PhotoBytesQuota     float,
USER_HierarchyOn         int,

primary key (USER_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  pivot table relating PhotoSets to specific Categories
##

create table p_psCategories (
PS_ID                    int not null,
CAT_Code                 int not null default -1,

primary key (PS_ID, CAT_Code)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  pivot table relating Photos to specific Categories
##

create table p_pCategories (
P_ID                     int not null,
CAT_Code                 int not null default -1,

primary key (P_ID, CAT_Code)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  pivot table relating PhotoSets to specific ParentGroups
##

create table p_psParentGroup (
PAR_ID                    int not null,  ##  the parent group
PAR_PS_ID                 int not null,  ##  the photoset related to
PAR_GRP_ID                int not null,  ##  the group related to

primary key (PAR_ID)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  temporary table to hold the photoset id's - these are discovered
##  while loading the group hierarchy but need to be queried to get
##  the images in them in a later step
##

create table tmp_PhotoSet (
tps_Id                    int not null,

primary key (tps_Id)
)
engine = MyISAM
data directory = '${DBdatadir}'
index directory = '${DBindexdir}'
;

##
##  enum types
##

#GroupShiftOrder          enum ('CreatedAsc', 'CreatedDesc', 'ModifiedAsc', 'ModifiedDesc', 'TitleAsc', 'TitleDesc', 'GroupsTop', 'GroupsBottom'),
#ShiftOrder               enum ('CreatedAsc', 'CreatedDesc', 'TakenAsc', 'TakenDesc', 'TitleAsc', 'TitleDesc', 'SizeAsc', 'SizeDesc', 'FileNameAsc', 'FileNameDesc'),
#SortOrder                enum ('Date', 'Popularity', 'Rank'),

EOF

##
##  tables are ready - let's get some authoriziation and load the tables
##

##  remove files from previous runs

rm -f AuthToken.xml AuthToken                                   >/dev/null 2>&1
rm -f XML*.xml                                                  >/dev/null 2>&1
rm -f BringCurrent.sql                                          >/dev/null 2>&1
rm -f /tmp/dougee* /tmp/Galleries.*                             >/dev/null 2>&1
rm -f /tmp/Photos.*                                             >/dev/null 2>&1
rm -f /tmp/CSV-GalleryInfo.*                                    >/dev/null 2>&1

##  make sure required support files exist

echo "Building/Updating required support files..."

make  >/dev/null 2>&1
( cd $ProgPrefix ; make )  >/dev/null 2>&1

##  get auth token for zenfolio

echo "Getting authentication credentials..."

./GetAuthToken | tail -2 > AuthToken.xml
chmod 0400 AuthToken.xml
AuthToken=`./ParseXMLFile AuthToken.xml | awk '{ print $2 }'`
echo ${AuthToken} > AuthToken
chmod 0400 AuthToken

##  exit here to just build DB and progs ##  

#exit

##
##  load the stand-alone tables - these are the tables of information
##  that zenfolio maintains and provides the structures of the galleries
##

echo
echo "Loading ZenFolio managed data..."

##  load the categories

Res=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from categoryinfo;
EOF
) | tail -1`
echo -n "  Loading Categories $Res --> "

./LoadCategoryTable `cat AuthToken`

Res=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from categoryinfo;
EOF
) | tail -1`
echo "$Res"

##
##  load the user tables - Groups, PhotoSets, Photos, etc
##

echo
echo "Loading user specific data..."

##  load the GroupHierarchy and save the PhotoSet Id's

GHStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from psGroup;
EOF
) | tail -1`

GalStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from Gallery;
EOF
) | tail -1`

GEStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from GroupElement;
EOF
) | tail -1`

PSStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from tmp_PhotoSet;
EOF
) | tail -1`

ADStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from AccessDescriptor;
EOF
) | tail -1`

AMStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from AccessMask;
EOF
) | tail -1`

PHStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from Photo;
EOF
) | tail -1`

PFStart=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from PhotoFlags;
EOF
) | tail -1`


./LoadGroups `cat AuthToken` # >/dev/null 2>&1


GHEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from psGroup;
EOF
) | tail -1`

GalEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from Gallery;
EOF
) | tail -1`

GEEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from GroupElement;
EOF
) | tail -1`

PSEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from tmp_PhotoSet;
EOF
) | tail -1`

ADEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from AccessDescriptor;
EOF
) | tail -1`

AMEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from AccessMask;
EOF
) | tail -1`

PHEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from Photo;
EOF
) | tail -1`

PFEnd=`(
${MYSQL} <<EOF
use zenfolio;
select count(*) from PhotoFlags;
EOF
) | tail -1`

echo
echo "DB Group elements updated:"
echo "  PSGroup:          $GHStart --> $GHEnd"
echo "  GroupElement:     $GEStart --> $GEEnd"
echo "  Galleries:        $GalStart --> $GalEnd"
echo "  PhotoSets:        $PSStart --> $PSEnd"
echo "  Photos:           $PHStart --> $PHEnd"
echo "  PhotoFlags:       $PFStart --> $PFEnd"
echo "  AccessDescriptor: $ADStart --> $ADEnd"
echo "  AccessMask:       $AMStart --> $AMEnd"

##
##  we now have catagories and gallery information in the database.
##  we also have a some specific photo information from the TitlePhotos but
##  it is not complete.
##
##  loop through the galleries, get the specific photo information, and 
##  load the database with photo specific information
##
##  the Gallery table has the number of photos in each gallery
##  use LoadPhotoSetPhotos to get the gallery's detailed photo information
##

echo
echo "Loading Photo information by Gallery..."

${ProgPrefix}/CSVQuery zenfolio "select gal_id, gal_title, gal_photocount from gallery order by gal_title" > /tmp/CSV-GalleryInfo.$$

exec 4</tmp/CSV-GalleryInfo.$$
while read -u4 GalleryString
do
  GallID=`echo $GalleryString | awk -F \, '{ print $1 }'`
  GallName=`echo $GalleryString | awk -F \, '{ print $2 }'`
  PhotoCount=`echo $GalleryString | awk -F \, '{ print $3 }'`
  echo
  echo "  Processing $PhotoCount images from $GallName"
  ./LoadPhotosByGallery `cat AuthToken` $GallID $PhotoCount
#  ./LoadPhotosByGallery `cat AuthToken` $GallID 2
done  ##  while reading gallery information
exec 4<&-

#rm -f /tmp/CSV-GalleryInfo.$$
