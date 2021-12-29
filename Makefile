
all: pixelspam pngspam pixelflame

pixelspam: pixelspam.c util.c
	$(CC) $(CFLAGS) -o $@ $^ -pthread -lm -lrt

pngspam: pngspam.c util.c
	$(CC) $(CFLAGS) -o $@ $^ -lpng -pthread

pixelflame: pixelflame.c util.c
	$(CC) $(CFLAGS) -o $@ $^ -pthread -lm -lrt

