# makfile configuration
CPU             	= msp430g2553
CFLAGS          	= -mmcu=${CPU} -Os -I../h -I../h

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-gcc -mmcu=${CPU} -c 

all:Pong.elf

#additional rules for files
Pong.elf: ${COMMON_OBJECTS} Pong.o wdt_handler.o buzzerFunctions.o
	${CC} -mmcu=${CPU}   -o $@ $^ -L../lib -lTimer -lLcd -lShape -lCircle -lp2sw

load: Pong.elf
	mspdebug rf2500 "prog $^"

clean:
	rm -f *.o *.elf
