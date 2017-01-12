#
#  Makefile for the Daemony Database Updater
#

##
##  some reference info
##
##  LICL gallery id:  428438940


CC= gcc

CFLAGS= -DDOUNLINK

##
##  for debugging
##

#  generally enough to debug execution flow problems

# DEBUGFLAGS= -DDUMPDATA -DDEBUG
# DEBUGFLAGS= -DDEBUG -DDBDEBUG # -DSHOWFOUND -DMEMDEBUG

#  to show the details of the XML parsing add -DSHOWFOUND
#  to debug database issues add -DDBDEBUG
#  to debug memory allocation issues add -DMEMDEBUG
#  to dump the data structures only add -DDUMPDATA


INCLUDEDIRS= -I/usr/GNU/include/libxml2
LINKOPTS= 

##
##  openssl specific options
##

SSLLIBS= -lssl -lcrypto -lz

##
##  xml specific options
##

XMLLIBS= -lxml2

##
##  mysql specific options
##

MYSQLINC= -I/usr/local/mysql/include
MYSQLLIBS= -L/usr/local/mysql/lib -lmysqlclient # -lz -lm

##

##
##  NO USER SERVICEABLE PARTS BEYOND THIS POINT
##

Targets= GetAuthToken ParseXMLFile GetPhotoInfoXML GetAllGalleriesXML \
	ParseAllGalleries GetPhotoSetXML ParsePhotoSet ShowViews \
	LoadCategoryTable LoadGroups LoadPhotosByGallery
TestTargets= tree1 Test-SetKeyCat

#  for the LoadGroups prog

LGFunctions= LG-ProcAM.o LG-ProcPF.o \
	LG-AddAD.o LG-AddGal.o LG-AddPhoto.o LG-AddTitlePhoto.o

#  for the LoadPhotosByGallery prog

LPBGFunctions= LPBG-AddPhoto.o LPBG-AddPhotoId.o LPBG-AddCat.o \
	LPBG-AddKey.o LPBG-AddETags.o LPBG-AddPhotoInDB.o \
	LPBG-AddPFInDB.o \
	LPBG-ProcPF.o \
	LPBG-IsPhotoInDB.o LPBG-IsPFInDB.o

#  generally useful stuff goes into a fake library

MyXMLLibTargets= NextTokenTrim.o \
	BC-LoadPhoto.o BC-LoadGroupHier.o \
	BH-LoadPhoto.o BH-LoadGroupHier.o


.c.o:
	$(CC) -c ${CFLAGS} ${COPTS} ${INCLUDEDIRS} ${MYSQLINC} ${DEBUGFLAGS} $<



all: ${Targets}

tests: ${TestTargets}


LoadCategoryTable: LoadCategoryTable.o
	$(CC) -o LoadCategoryTable LoadCategoryTable.o ${MYSQLINC} ${LINKOPTS} ${XMLLIBS} ${SSLLIBS} ${MYSQLLIBS}
	chmod 0755 LoadCategoryTable && echo

LoadGroups: myxml.o LoadGroups.o ${LGFunctions}
	$(CC) -o LoadGroups LoadGroups.o ${MYSQLINC} ${LINKOPTS} ${XMLLIBS} ${SSLLIBS} ${MYSQLLIBS} ${LGFunctions} myxml.o
	chmod 0755 LoadGroups && echo

LoadPhotosByGallery: myxml.o LoadPhotosByGallery.o ${LPBGFunctions}
	$(CC) -o LoadPhotosByGallery LoadPhotosByGallery.o ${MYSQLINC} ${LINKOPTS} ${XMLLIBS} ${SSLLIBS} ${MYSQLLIBS} ${LPBGFunctions} myxml.o
	chmod 0755 LoadPhotosByGallery && echo


GetAuthToken: GetAuthToken.o
	$(CC) -o GetAuthToken GetAuthToken.o ${LINKOPTS} ${SSLLIBS}
	chmod 0755 GetAuthToken && echo

GetPhotoInfoXML: GetPhotoInfoXML.o
	$(CC) -o GetPhotoInfoXML GetPhotoInfoXML.o ${LINKOPTS} ${SSLLIBS}
	chmod 0755 GetPhotoInfoXML && echo

GetAllGalleriesXML: GetAllGalleriesXML.o
	$(CC) -o GetAllGalleriesXML GetAllGalleriesXML.o ${LINKOPTS} ${SSLLIBS}
	chmod 0755 GetAllGalleriesXML && echo

GetPhotoSetXML: GetPhotoSetXML.o
	$(CC) -o GetPhotoSetXML GetPhotoSetXML.o ${LINKOPTS} ${SSLLIBS}
	chmod 0755 GetPhotoSetXML && echo

Test-SetKeyCat: Test-SetKeyCat.o
	$(CC) -o Test-SetKeyCat Test-SetKeyCat.o ${LINKOPTS} ${SSLLIBS}
	chmod 0755 Test-SetKeyCat && echo

ParseXMLFile: ParseXMLFile.o
	$(CC) -o ParseXMLFile ParseXMLFile.o ${LINKOPTS} ${XMLLIBS}
	chmod 0755 ParseXMLFile && echo

ParseAllGalleries: ParseAllGalleries.o
	$(CC) -o ParseAllGalleries ParseAllGalleries.o ${LINKOPTS} ${XMLLIBS}
	chmod 0755 ParseAllGalleries && echo

ShowViews: ShowViews.o
	$(CC) -o ShowViews ShowViews.o ${LINKOPTS} ${XMLLIBS} ${SSLLIBS}
	chmod 0755 ShowViews && echo

ParsePhotoSet: ParsePhotoSet.o
	$(CC) -o ParsePhotoSet ParsePhotoSet.o ${LINKOPTS} ${XMLLIBS}
	chmod 0755 ParsePhotoSet && echo



tree1: tree1.o
	$(CC) -o tree1 tree1.o ${LINKOPTS} ${XMLLIBS}
	chmod 0755 tree1 && echo

##
##  program specific support code
##

LG-ProcAM.o: LG-ProcAM.c
	$(CC) -c -o LG-ProcAM.o LG-ProcAM.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LG-ProcPF.o: LG-ProcPF.c
	$(CC) -c -o LG-ProcPF.o LG-ProcPF.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LG-AddAD.o: LG-AddAD.c
	$(CC) -c -o LG-AddAD.o LG-AddAD.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LG-AddGal.o: LG-AddGal.c
	$(CC) -c -o LG-AddGal.o LG-AddGal.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LG-AddPhoto.o: LG-AddPhoto.c
	$(CC) -c -o LG-AddPhoto.o LG-AddPhoto.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LG-AddTitlePhoto.o: LG-AddTitlePhoto.c
	$(CC) -c -o LG-AddTitlePhoto.o LG-AddTitlePhoto.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}


LPBG-AddPhoto.o: LPBG-AddPhoto.c
	$(CC) -c -o LPBG-AddPhoto.o LPBG-AddPhoto.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-AddPhotoId.o: LPBG-AddPhotoId.c
	$(CC) -c -o LPBG-AddPhotoId.o LPBG-AddPhotoId.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-AddPhotoInDB.o: LPBG-AddPhotoInDB.c
	$(CC) -c -o LPBG-AddPhotoInDB.o LPBG-AddPhotoInDB.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-AddPFInDB.o: LPBG-AddPFInDB.c
	$(CC) -c -o LPBG-AddPFInDB.o LPBG-AddPFInDB.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-AddCat.o: LPBG-AddCat.c
	$(CC) -c -o LPBG-AddCat.o LPBG-AddCat.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-AddKey.o: LPBG-AddKey.c
	$(CC) -c -o LPBG-AddKey.o LPBG-AddKey.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-AddETags.o: LPBG-AddETags.c
	$(CC) -c -o LPBG-AddETags.o LPBG-AddETags.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-IsPhotoInDB.o: LPBG-IsPhotoInDB.c
	$(CC) -c -o LPBG-IsPhotoInDB.o LPBG-IsPhotoInDB.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-IsPFInDB.o: LPBG-IsPFInDB.c
	$(CC) -c -o LPBG-IsPFInDB.o LPBG-IsPFInDB.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

LPBG-ProcPF.o: LPBG-ProcPF.c
	$(CC) -c -o LPBG-ProcPF.o LPBG-ProcPF.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}


##
##  my general XML helper library
##

myxml.o: ${MyXMLLibTargets}
	ld -r -o myxml.o ${MyXMLLibTargets} && echo
#	libtool -v --mode=link gcc -o libmyxml.a ${MyXMLLibTargets}
#	ranlib libmyxml.a && echo
#	ar -rcs libmyxml.a ${MyXMLLibTargets}
#	ar -rcs --target=mach-o-x86-64 libmyxml.a ${MyXMLLibTargets}
#	ranlib libmyxml.a && echo

NextTokenTrim.o: NextTokenTrim.c
	$(CC) -c -o NextTokenTrim.o NextTokenTrim.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

BC-LoadPhoto.o: BC-LoadPhoto.c
	$(CC) -c -o BC-LoadPhoto.o BC-LoadPhoto.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

BC-LoadGroupHier.o: BC-LoadGroupHier.c
	$(CC) -c -o BC-LoadGroupHier.o BC-LoadGroupHier.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

BH-LoadPhoto.o: BH-LoadPhoto.c
	$(CC) -c -o BH-LoadPhoto.o BH-LoadPhoto.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

BH-LoadGroupHier.o: BH-LoadGroupHier.c
	$(CC) -c -o BH-LoadGroupHier.o BH-LoadGroupHier.c ${CFLAGS} ${COPTS} ${DEBUGFLAGS}

clean:
	/bin/rm -f *.o *~ \#* core
	/bin/rm -f ${Targets} ${TestTargets}
	/bin/rm -f XML-*.xml
	/bin/rm -f myxml.o && echo

backup: clean
	rm -f  ../ZenFolio.tar.gz
	tar cf ../ZenFolio.tar *.c *.h *.ksh Makefile
	gzip   ../ZenFolio.tar && echo

##
##  not used here but kept as a library creation example
##

libzen.a: ${LibTargets}
	libtool -o libzen.a ${LibTargets}
	ranlib libzen.a && echo

#
#  depend relationships
#

GetAuthToken.o: GetAuthToken.c
GetPhotoInfoXML.o: GetPhotoInfoXML.c
GetAllGalleriesXML.o: GetAllGalleriesXML.c
GetPhotoSetXML.o: GetPhotoSetXML.c
ParseXMLFile.o: ParseXMLFile.c
ParseAllGalleries.o: ParseAllGalleries.c
ParsePhotoSet.o: ParsePhotoSet.c
Test-SetKeyCat.o: Test-SetKeyCat.c
ShowViews.o: ShowViews.c
LoadCategoryTable.o: LoadCategoryTable.c /tmp/DBvip

NextTokenTrim.o: NextTokenTrim.c
BC-LoadPhoto.o: BC-LoadPhoto.c
BC-LoadGroupHier.o: BC-LoadGroupHier.c
BH-LoadPhoto.o: BH-LoadPhoto.c
BH-LoadGroupHier.o: BH-LoadGroupHier.c

LoadGroups.o: LoadGroups.c ZenFolio.h LoadGroups.h /tmp/DBvip
LG-AddAD.o: LG-AddAD.c LoadGroups.h Zenfolio.h
LG-AddGal.o: LG-AddGal.c LoadGroups.h Zenfolio.h
LG-AddPhoto.o: LG-AddPhoto.c LoadGroups.h Zenfolio.h
LG-AddTitlePhoto.o: LG-AddTitlePhoto.c LoadGroups.h Zenfolio.h
LG-ProcAM.o:  LG-ProcAM.c LoadGroups.h Zenfolio.h
LG-ProcPF.o:  LG-ProcPF.c LoadGroups.h Zenfolio.h

LoadPhotosByGallery.o: LoadPhotosByGallery.c ZenFolio.h LoadPhotosByGallery.h /tmp/DBvip
LPBG-AddPhoto.o: LPBG-AddPhoto.c LoadPhotosByGallery.h ZenFolio.h
LPBG-AddPhotoId.o: LPBG-AddPhotoId.c LoadPhotosByGallery.h ZenFolio.h
LPBG-AddPhotoInDB.o: LPBG-AddPhotoInDB.c LoadPhotosByGallery.h ZenFolio.h
LPBG-AddPFInDB.o: LPBG-AddPFInDB.c LoadPhotosByGallery.h ZenFolio.h
LPBG-AddCat.o: LPBG-AddCat.c LoadPhotosByGallery.h ZenFolio.h
LPBG-AddKey.o: LPBG-AddKey.c LoadPhotosByGallery.h ZenFolio.h
LPBG-AddETags.o: LPBG-AddETags.c LoadPhotosByGallery.h ZenFolio.h
LPBG-ProcPF.o: LPBG-ProcPF.c LoadPhotosByGallery.h ZenFolio.h
LPBG-IsPhotoInDB.o: LPBG-IsPhotoInDB.c LoadPhotosByGallery.h ZenFolio.h
LPBG-IsPFInDB.o: LPBG-IsPFInDB.c LoadPhotosByGallery.h ZenFolio.h
