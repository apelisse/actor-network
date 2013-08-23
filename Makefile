CFLAGS = -Wextra -W -Wno-unused-result -O3 -g

-include config.mak

CFLAGS_ALL = $(CFLAGS) $(shell pkg-config --cflags glib-2.0)
LDFLAGS_ALL = $(LDFLAGS) $(shell pkg-config --libs glib-2.0)

all:: bacon importer

bacon: pack.o bacon.o
	$(CC) $^ -o $@ $(LDFLAGS_ALL)

importer: importer.o
	$(CC) $^ -o $@ $(LDFLAGS_ALL)

.c.o:
	$(CC) $< -c -o $@ $(CFLAGS_ALL)

.PHONE: clean
clean:
	-$(RM) bacon importer *.o
