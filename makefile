#
# Parameters
#===========

APPNAME     = can
CXX         = avr-gcc
CXXFLAGS    = -Wall
LIBS        = -Iinc
DEVICE		= at90can128
FCPU		= 16000000
OFILES		= test.o can.o

#
# Make
#===========

main: $(OFILES)
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -o bin/$(APPNAME) $(OFILES)

test.o: src/test.c
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -c src/test.c $(LIBS)

can.o: src/can.c
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -c src/can.c $(LIBS)

clean:
	rm *.o