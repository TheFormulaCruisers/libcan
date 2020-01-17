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

CAN_O = can_test.o can.o
CAN_BIN = can_test

can_build: $(CAN_O)
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -o bin/$(CAN_BIN) $(CAN_O)

can_test.o: tests/can_test.c
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -c tests/can_test.c $(LIBS)

can.o: src/can.c
	$(CXX) $(CXXFLAGS) -mmcu=$(DEVICE) -DF_CPU=$(FCPU) -DCAN_REV_2B -DCAN_RX_MSGBUF_SIZE=14 -DCAN_TX_MSGBUF_SIZE=8 -c src/can.c $(LIBS)

clean:
	rm *.o