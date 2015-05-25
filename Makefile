vbswitch: vbswitch.c
	cc -o vbswitch vbswitch.c

clean:
	rm vbswitch

install:
	cp vbswitch /usr/bin
	chmod a+rx /usr/bin/vbswitch

uninstall:
	rm /usr/bin/vbswitch
