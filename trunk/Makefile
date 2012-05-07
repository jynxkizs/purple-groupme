#Customisable stuff here
LINUX32_COMPILER = gcc #i686-pc-linux-gnu-gcc
LINUX64_COMPILER = x86_64-linux-gnu-gcc
WIN32_COMPILER = gcc.exe #/usr/bin/i586-mingw32msvc-gcc
WIN32_WINDRES = i586-mingw32msvc-windres
#LINUX_ARM_COMPILER = arm-pc-linux-gnu-gcc
LINUX_ARM_COMPILER = arm-linux-gnueabi-gcc
LINUX_PPC_COMPILER = powerpc-unknown-linux-gnu-gcc
FREEBSD60_COMPILER = i686-pc-freebsd6.0-gcc
MACPORT_COMPILER = i686-apple-darwin10-gcc-4.0.1

LIBPURPLE_CFLAGS = -I/usr/include/libpurple -I/usr/local/include/libpurple -I./../.. -DPURPLE_PLUGINS -DENABLE_NLS -DHAVE_ZLIB
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/local/include # -I/usr/include/json-glib-1.0 
#$(pkg-config --cflags glib-2.0)
#-ljson-glib-1.0
WIN32_DEV_DIR = ../../../../win32-dev
WIN32_PIDGIN_DIR = ../../../../pidgin-2.7.11
WIN32_GTK_DIR = ${WIN32_DEV_DIR}/gtk_2_0-2.22.1
WIN32_CFLAGS = -I${WIN32_GTK_DIR}/include/glib-2.0 -I${WIN32_PIDGIN_DIR}/libpurple/win32 -I${WIN32_GTK_DIR}/include -I${WIN32_GTK_DIR}/include/glib-2.0 -I${WIN32_GTK_DIR}/lib/glib-2.0/include -I/usr/include/json-glib-1.0
WIN32_LIBS = -L${WIN32_GTK_DIR}/lib -L${WIN32_PIDGIN_DIR}/libpurple -lglib-2.0 -lgobject-2.0 -lintl -lpurple -lws2_32 -L. -lz
#-lzlib1
MACPORT_CFLAGS = -I/opt/local/include/libpurple -DPURPLE_PLUGINS -DENABLE_NLS -DHAVE_ZLIB -I/opt/local/include/glib-2.0 -I/opt/local/lib/glib-2.0/include -I/opt/local/include -I/opt/local/include/json-glib-1.0 -arch i386 -arch ppc -dynamiclib -L/opt/local/lib -ljson-glib-1.0 -lpurple -lglib-2.0 -lgobject-2.0 -lintl -lz -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4

DEB_PACKAGE_DIR = ./debdir

GROUPME_SOURCES =  libgroupme.c \
		   libgroupme.h \
		   GroupMeAccount.c \
		   GroupMeAccount.h \
		   GroupMeLog.h \
		   GroupMePod.c \
		   GroupMePod.h \
		   GroupMeProtocol.c \
		   GroupMeProtocol.h \
		   GroupMeUpdate.c \
		   GroupMeUpdate.h \
		   groupme_connection.c \
		   groupme_connection.h \
		   groupme_json.c \
		   groupme_json.h \
		   groupme_html.c \
	 	   groupme_html.h

#Standard stuff here
.PHONY:	all clean install sourcepackage

groupme:	libgroupme.so
	cp libgroupme.so ~/.purple/plugins/

groupme64:
	libgroupme64.so
	cp libgroupme64.so ~/.purple/plugins/

all:	libgroupme.so libgroupme.dll libgroupme64.so libgroupmearm.so libgroupmeppc.so installers sourcepackage
all:	libgroupme.so libgroupme.dll

install:
	cp libgroupme.so /usr/lib/purple-2/
	cp libgroupme64.so /usr/lib64/purple-2/
	cp groupme16.png /usr/share/pixmaps/pidgin/protocols/16/groupme.png
	cp groupme22.png /usr/share/pixmaps/pidgin/protocols/22/groupme.png
	cp groupme48.png /usr/share/pixmaps/pidgin/protocols/48/groupme.png
	cp groupme.svg /usr/share/pixmaps/pidgin/protocols/scalable/groupme.svg
#cp libgroupmearm.so /usr/lib/pidgin/
#cp libgroupmeppc.so /usr/lib/purple-2/

installers:	purple-groupme.exe purple-groupme.deb purple-groupme.tar.bz2

clean:
	rm -f libgroupme.so libgroupme.dll libgroupme64.so libgroupmearm.so libgroupmeppc.so purple-groupme.exe purple-groupme.deb purple-groupme.tar.bz2 purple-groupme-source.tar.bz2
	rm -rf purple-groupme

libgroupme.so:	${GROUPME_SOURCES}
	${LINUX32_COMPILER} ${LIBPURPLE_CFLAGS} -Wall ${GLIB_CFLAGS} -I. -g -O2 -pipe ${GROUPME_SOURCES} -o libgroupme.so -shared -fPIC -DPIC

libgroupmearm.so:	${GROUPME_SOURCES}
	${LINUX_ARM_COMPILER} ${LIBPURPLE_CFLAGS} -Wall ${GLIB_CFLAGS} -I. -g -O2 -pipe ${GROUPME_SOURCES} -o libgroupmearm.so -shared -fPIC -DPIC

libgroupme64.so:	${GROUPME_SOURCES}
	${LINUX64_COMPILER} ${LIBPURPLE_CFLAGS} -Wall ${GLIB_CFLAGS} -I. -g -m64 -O2 -pipe ${GROUPME_SOURCES} -o libgroupme64.so -shared -fPIC -DPIC

libgroupmeppc.so:	${GROUPME_SOURCES}
	${LINUX_PPC_COMPILER} ${LIBPURPLE_CFLAGS} -Wall ${GLIB_CFLAGS} -I. -g -O2 -pipe ${GROUPME_SOURCES} -o libgroupmeppc.so -shared -fPIC -DPIC

libgroupmemacport.so: ${GROUPME_SOURCES}
	${MACPORT_COMPILER} ${MACPORT_CFLAGS} -Wall -I. -g -O2 -pipe ${GROUPME_SOURCES} -o libgroupmemacport.so -shared
#
#purple-groupme.res:	purple-groupme.rc
#	${WIN32_WINDRES} $< -O coff -o $@

libgroupme.dll:	${GROUPME_SOURCES}
	${WIN32_COMPILER} ${LIBPURPLE_CFLAGS} -Wall -I. -g -O2 -pipe ${GROUPME_SOURCES} -o $@ -shared -mno-cygwin ${WIN32_CFLAGS} ${WIN32_LIBS} -Wl,--strip-all
	upx libgroupme.dll

libgroupme-debug.dll:	${GROUPME_SOURCES}
	${WIN32_COMPILER} ${LIBPURPLE_CFLAGS} -Wall -I. -g -O2 -pipe ${GROUPME_SOURCES} -o $@ -shared -mno-cygwin ${WIN32_CFLAGS} ${WIN32_LIBS}
#
#libgroupmebsd60.so:	${GROUPME_SOURCES}
#	${FREEBSD60_COMPILER} ${LIBPURPLE_CFLAGS} -Wall ${GLIB_CFLAGS} -I. -g -O2 -pipe ${GROUPME_SOURCES} -o libgroupme.so -shared -fPIC -DPIC


purple-groupme.exe:	libgroupme.dll
	echo "Dont forget to update version number"
	makensis groupme.nsi > /dev/null

purple-groupme.deb:	libgroupme.so libgroupmearm.so libgroupme64.so libgroupmeppc.so
	echo "Dont forget to update version number"
	cp libgroupme.so ${DEB_PACKAGE_DIR}/usr/lib/purple-2/
	cp libgroupmeppc.so ${DEB_PACKAGE_DIR}/usr/lib/purple-2/
	cp libgroupme64.so ${DEB_PACKAGE_DIR}/usr/lib64/purple-2/
	cp libgroupmearm.so ${DEB_PACKAGE_DIR}/usr/lib/pidgin/
	cp groupme16.png ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/16/groupme.png
	cp groupme22.png ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/22/groupme.png
	cp groupme48.png ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/48/groupme.png
	cp groupme.svg ${DEB_PACKAGE_DIR}/usr/share/pixmaps/pidgin/protocols/scalable/groupme.svg
	chown -R root:root ${DEB_PACKAGE_DIR}
	chmod -R 755 ${DEB_PACKAGE_DIR}
	dpkg-deb --build ${DEB_PACKAGE_DIR} $@ > /dev/null

purple-groupme.tar.bz2:	purple-groupme.deb
	tar --bzip2 --directory ${DEB_PACKAGE_DIR} -cf $@ usr/

sourcepackage:	${GROUPME_SOURCES} Makefile groupme16.png groupme22.png groupme48.png groupme.svg COPYING groupme.nsi
	tar -cf tmp.tar $^
	mkdir purple-groupme
	mv tmp.tar purple-groupme
	tar xvf purple-groupme/tmp.tar -C purple-groupme
	rm purple-groupme/tmp.tar
	tar --bzip2 -cf purple-groupme-source.tar.bz2 purple-groupme
	rm -rf purple-groupme
