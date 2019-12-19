#
# Parameters
#===========

CXX         = avr-gcc
CXXFLAGS    = -Wall
LIBS        = -Iinc
DEVICE		= at90can128
FCPU		= 16000000

#
# Make
#===========

CAN_O = can_tests.o can.o
CAN_BIN = can_tests

can_build: $(CAN_O)
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -o bin/$(CAN_BIN) $(CAN_O)

can_tests.o: tests/can_tests.c
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -c tests/can_tests.c $(LIBS)

can.o: src/can.c
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -c src/can.c $(LIBS)

clean:
	rm *.o