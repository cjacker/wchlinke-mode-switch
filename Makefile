DESTDIR=

all:
	gcc -o wchlinke-mode-switch main.c `pkg-config --libs --cflags libusb-1.0`

install: all
	mkdir -p $(DESTDIR)/usr/bin
	install -m0755 wchlinke-mode-switch $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/etc/udev/rules.d
	install -m0644 99-wch-link.rules $(DESTDIR)/etc/udev/rules.d	

clean:
	rm -f wchlinke-mode-switch
