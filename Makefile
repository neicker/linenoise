override CFLAGS += -Wall -W -Os -g

TARGETS = linenoise_example linenoise_async

all: $(TARGETS)

linenoise.o example.o async.o: linenoise.h

linenoise_example: linenoise.o example.o
	$(CC) -o $@ $^

asyncexample.o: CFLAGS += -std=gnu99

linenoise_async: linenoise.o asyncexample.o
	$(CC) -o $@ $^


clean:
	rm -f $(TARGETS)
