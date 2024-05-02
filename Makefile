CC = gcc
CXX = g++

CFLAGS = 
LIBFLAGS = -lpthread -lwiringPi

EXEC = mplayerRadio
SRC = ./*.c

LOOP_EXEC_DIR = /home/wiselot/Radio/mplayerRadio
LOOP_BASH = mplayerRadio_loop.sh

all:
	echo "[Start to compile...]"
	$(CC) -o $(EXEC) $(SRC) $(CFLAGS) $(LIBFLAGS)
	echo "[Compile done]"

install: $(EXEC)
	echo "[Start to install]"
	cp $(EXEC) $(LOOP_EXEC_DIR)
	cp $(LOOP_BASH) /etc/init.d/
	echo "[Install done]"

uninstall:
	echo "[Start to install]"
	rm $(EXEC) $(LOOP_EXEC_DIR)
	rm /etc/init.d/$(LOOP_BASH)
	echo "[Uninstall done]"

clean:
	rm $(EXEC)
