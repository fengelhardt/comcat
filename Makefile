CC=gcc
CFLAGS=-g3 -O0

MODULES=main.o
PROG_NAME=comcat
INSTALL_DIR=/usr/local/bin

all : $(PROG_NAME)

$(PROG_NAME) : $(MODULES)
	$(CC) $(CFLAGS) $(MODULES) -o $(PROG_NAME)
	
%.o : %.c
	$(CC) $(CFLAGS) -o "$@" -c "$<"
	
clean : 
	rm *.o
	rm $(PROG_NAME)

install :
	cp $(PROG_NAME) $(INSTALL_DIR)/

uninstall:
	rm -f $(INSTALL_DIR)/$(PROG_NAME)
	
.PHONY : clean
