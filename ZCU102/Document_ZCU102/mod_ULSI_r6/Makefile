TARGET = mpcnn
SUBPRG = standalone
OPTLVL = -O3
GPROF  = -pg

ifeq ($(ARCH),arm)
	CC = arm-linux-gnueabihf-gcc
	CX = arm-linux-gnueabihf-g++
else
	CC = gcc
	CX = g++
endif

$(TARGET):main2.o main.o $(SUBPRG)
	$(CX) main.o main2.o -o $(TARGET) -static $(OPTLVL) $(GPROF)

main.o:main.cpp main.hpp RxTx.hpp
	$(CX) -c main.cpp -o main.o $(OPTLVL) $(GPROF)

main2.o:main.c
	$(CC) -c -DWITHOUT_MAIN main.c -o main2.o $(OPTLVL) -Wno-unused-result $(GPROF)

$(SUBPRG):main.c
	$(CC) main.c -o $(SUBPRG) -static $(OPTLVL) -Wall -Wno-unused-result $(GPROF)

clean:
	\rm main.o main2.o $(TARGET) $(SUBPRG)
