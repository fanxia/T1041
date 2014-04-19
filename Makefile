ROOTLIB=$(shell root-config --libdir)
ROOTCFLAGS=$(shell root-config --cflags)
ROOTLFLAGS=$(shell root-config --ldflags)
ROOTLIBS=$(shell root-config --libs) -lSpectrum -lGui
INC=../include
DEBUG=-ggdb
PROFILE=-pg
CXXFLAGS=-Wall -fPIC -O2 $(DEBUG) -rdynamic 


waveInterface.so: TBEvent.so waveInterface.o waveEventDict.so 
	g++ waveInterface.o -shared -Wl,-soname,waveInterface.so $(ROOTLFLAGS) -Wl,--no-undefined -Wl,--as-needed -L$(ROOTLIB) $(ROOTLIBS)  ./waveEventDict.so ./TBEvent.so ./TBEventDict.so $(ROOTLIBS) -o waveInterface.so

waveInterface.o: waveInterface.cc
	g++ -c -pipe -Wshadow -W -Woverloaded-virtual $(ROOTCFLAGS) $(CXXFLAGS) -DR__HAVE_CONFIG waveInterface.cc


TBEvent.so: TBEvent.o TBEventDict.so
	g++ -shared -o TBEvent.so $(ROOTCFLAGS) $(CXXFLAGS) TBEvent.o ./TBEventDict.so $(ROOTLIBS)

TBEvent.o: TBEvent.cc
	g++ -c -pipe -Wshadow -W -Woverloaded-virtual $(ROOTCFLAGS) $(CXXFLAGS) -DR__HAVE_CONFIG TBEvent.cc

TBEventDict.so: TBEventDict.cxx
	g++ -shared -o TBEventDict.so $(ROOTCFLAGS) $(CXXFLAGS) TBEventDict.cxx

TBEventDict.cxx: TBEvent.h LinkDef.h
	rootcint TBEventDict.cxx -c TBEvent.h LinkDef.h

waveEventDict.so: waveEventDict.cxx
	g++ -shared -o waveEventDict.so $(ROOTCFLAGS) $(CXXFLAGS) waveEventDict.cxx

waveEventDict.cxx: waveInterface.h waveLinkDef.h
	rootcint waveEventDict.cxx -c waveInterface.h waveLinkDef.h


clean:
	rm  -f *.so *.o *~ TBEventDict.cxx TBEventDict.h waveEventDict.cxx waveEventDict.h
