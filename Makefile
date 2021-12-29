
all: pixelspam pngspam

pixelspam: pixelspam.c util.c
	$(CC) -o $@ $^ -pthread -lm -lrt

pngspam: pngspam.c util.c
	$(CC) -o $@ $^ -lpng -pthread

