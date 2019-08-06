#-
#  ==========================================================================
#  Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
# 
#  Use of this software is subject to the terms of the Autodesk
#  license agreement provided at the time of installation or download,
#  or which otherwise accompanies this software in either electronic
#  or hard copy form.
#  ==========================================================================
#+
NAME =	tcSoftIkSolver
MAJOR =		1
MINOR =		0.0
VERSION =	$(MAJOR).$(MINOR)

LIBNAME =	$(NAME).so

C++			= g++412
MAYAVERSION = 2014
# Uncomment the following line on MIPSpro 7.2 compilers to supress
# warnings about declarations in the X header files
# WARNFLAGS	= -woff 3322

# When the following line is present, the linking of plugins will
# occur much faster.  If you link transitively (the default) then
# a number of "ld Warning 85" messages will be printed by ld.
# They are expected, and can be safely ignored.
MAYA_LOCATION = /usr/autodesk/maya$(MAYAVERSION)-x64/
INSTALL_PATH = /home/alan/projects/deploy/maya/tcSoftIkSolver/1.0/plug-ins/$(MAYAVERSION)/linux
CFLAGS		= -m64 -pthread -pipe -D_BOOL -DLINUX -DREQUIRE_IOSTREAM -Wno-deprecated -fno-gnu-keywords -fPIC 
C++FLAGS	= $(CFLAGS) $(WARNFLAGS)
INCLUDES	= -I. -I$(MAYA_LOCATION)/include -I/home/alan/projects/build/include/libxml2 -I/home/alan/projects/build/include -I/home/alan/projects/build/include/LicenseClient
LD			= $(C++) -shared $(C++FLAGS)
LIBS		= -L$(MAYA_LOCATION)/lib -lOpenMaya -lOpenMayaAnim -L/home/alan/projects/build/lib -lLicenseClient -lSockets -lssl -lcrypto -lpthread -ldl

.SUFFIXES: .cpp .cc .o .so .c

.cc.o:
	$(C++) -c $(INCLUDES) $(C++FLAGS) $<

.cpp.o:
	$(C++) -c $(INCLUDES) $(C++FLAGS) $<

.cc.i:
	$(C++) -E $(INCLUDES) $(C++FLAGS) $*.cc > $*.i

.cc.so:
	-rm -f $@
	$(LD) -o $@ $(INCLUDES) $< $(LIBS)

.cpp.so:
	-rm -f $@
	$(LD) -o $@ $(INCLUDES) $< $(LIBS)

.o.so:
	-rm -f $@
	$(LD) -o $@ $< $(LIBS)

CPP_FILES := $(wildcard *.cpp)
OBJS = $(notdir $(CPP_FILES:.cpp=.o))

all: $(LIBNAME)
	@echo "Done"
	mv $(LIBNAME) $(INSTALL_PATH)

$(LIBNAME): $(OBJS)
	-rm -f $@
	$(LD) -o $@ $(OBJS) $(LIBS) 

depend:
	makedepend $(INCLUDES) -I/usr/include/CC *.cc

clean:
	-rm -f *.o *.so

Clean:
	-rm -f *.o *.so *.bak
	
install:	all 
	mv $(LIBNAME) $(INSTALL_PATH)



