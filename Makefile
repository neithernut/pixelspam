
all: pixelspam pngspam

pixelspam: pixelspam.c util.c
	$(CC) $(CFLAGS) -o $@ $^ -pthread -lm -lrt

pngspam: pngspam.c util.c
	$(CC) $(CFLAGS) -o $@ $^ -lpng -pthread

