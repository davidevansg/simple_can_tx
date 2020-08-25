# Compilers
CC		= gcc
CXX		= g++

#Libraries

LIBS_PTHR	= -lpthread
LD_FLAGS	= $(LIBS_PTHR) -lm -I/usr/include

#CFlags
CFLAGS_STN	= -g -I$(INC_F) -I$(SRC_F)

CFLAGS_BLD	= -g $(CFLAGS_STN) -Wall -Werror
CFLAGS_DBG	= -g $(CFLAGS_STN)

#Application name
APP_NAME	= simple_can_tx

#Sources folder
SRC_F		= ./src
INC_F		= ./inc

SRC_C		= $(wildcard $(SRC_F)/*.c)

all: clean $(APP_NAME)

$(APP_NAME):
	$(CC) $(SRC_C) -o $@ $(CFLAGS_DBG) $(LD_FLAGS)

clean:
	rm -rf *.o
	rm -rf $(APP_NAME)
