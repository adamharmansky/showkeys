all: showkeys.c
	cc showkeys.c x.c -g -lX11 -lXinerama `pkg-config --libs --cflags cairo` -o showkeys

run: all
	./showkeys

debug: all
	gdb -tui ./showkeys

install: all
	install showkeys /usr/bin/
	mkdir -p /usr/share/showkeys
	cp -r keys/* /usr/share/showkeys

uninstall:
	rm -rf /usr/share/showkeys
	rm -rf /usr/bin/showkeys

clean:
	rm showkeys
