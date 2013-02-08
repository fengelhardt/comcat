CC=gcc
CFLAGS=-g3 -O0

MODULES=main.o
PROG_NAME=comcat

all : $(PROG_NAME)

$(PROG_NAME) : $(MODULES)
	$(CC) $(CFLAGS) $(MODULES) -o $(PROG_NAME)
	
%.o : %.c
	$(CC) $(CFLAGS) -o "$@" -c "$<"
	
clean : 
	rm *.o
	rm $(PROG_NAME)
	
.PHONY : clean