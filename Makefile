## Right now this Makefile is really brittle, if it fails during a build then you need to make clean; and restart 

SHELL=/bin/sh
.SUFFIXES:
.SUFFIXES: .cc .cxx .o
VPATH=src:include:build:build/lib:../src:/include:../include:lib
CC = g++
ROOTLIB=$(shell root-config --libdir)
ROOTCFLAGS=$(shell root-config --cflags)
ROOTLFLAGS=$(shell root-config --ldflags)
ROOTLIBS=$(shell root-config --libs) -lSpectrum -lGui
DEBUG=-ggdb
PROFILE=-pg
CXXFLAGS=-Wall -fPIC -O2 $(DEBUG) -rdynamic 
SHAREDCFLAGS = -shared $(ROOTCFLAGS) $(CXXFLAGS)
NONSHARED = -c -pipe -Wshadow -W -Woverloaded-virtual $(ROOTCFLAGS) $(CXXFLAGS) -DR__HAVE_CONFIG
BUILDDIR = $(CURDIR)/build
LIB = $(BUILDDIR)/lib

waveInterface.so: waveEventDict.so TBEvent.so waveInterface.o TBEventDict.so
	g++ -shared -Wl,-soname,waveInterface.so $(ROOTLFLAGS) -Wl,--no-undefined -Wl,--as-needed -L$(ROOTLIB) $(ROOTLIBS) $(filter-out %.so, $^) $(abspath $(patsubst %.so, $(LIB)/%.so,$(filter %.so, $(notdir $^)))) $(ROOTLIBS) -o waveInterface.so
	mv $@ $(BUILDDIR)


waveEventDict.so: waveEventDict.cxx 
	$(CC) $(SHAREDCFLAGS) -I. $^ -o $@
	test -d $(LIB) || mkdir -p $(LIB)
	mv $@ $(LIB)/

waveEventDict.cxx: waveInterface.h waveLinkDef.h
	rm -f ./waveEventDict.cxx ./waveEventDict.h
	rootcint $@ -c $^

waveInterface.o: waveInterface.cc
	$(CC) -I$(<D)/../include $(NONSHARED) $^


TBEvent.so: TBEvent.o TBEventDict.so
	$(CC) $(SHAREDCFLAGS) -o $@ $(abspath $(patsubst %.so, $(LIB)/%.so,$^)) $(ROOTLIBS)
	mv $@ $(LIB)/	

TBEvent.o: TBEvent.cc 
	$(CC) -I$(<D)/../include $(NONSHARED) $^

TBEventDict.so: TBEventDict.cxx
	$(CC) $(SHAREDCFLAGS) -I. $^ -o $@ 
	mv $@ $(LIB)/

TBEventDict.cxx: TBEvent.h LinkDef.h
	rm -f ./TBEventDict.cxx ./TBEventDict.h
	rootcint $@ -c $^ 



clean:
	rm  -f *.so *.o *~ TBEventDict.cxx TBEventDict.h waveEventDict.cxx waveEventDict.h $(LIB)/* $(BUILDDIR)/waveInterface.so

