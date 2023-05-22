CFLAGS += $(INCLUDES) -O -Wall -std=gnu99

all: ayplayer

ayplayer: ayplayer.o arduino-serial-lib.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

