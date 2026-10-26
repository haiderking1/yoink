CC      = clang
PREFIX ?= $(HOME)/.local

APP_PKGS  = libadwaita-1 json-glib-1.0
HOST_PKGS = gio-2.0 gio-unix-2.0 json-glib-1.0

CFLAGS ?= -O2 -g
CFLAGS += -std=gnu23 -Wall -Wextra -Wno-unused-parameter -MMD -MP

APP_CFLAGS  = $(CFLAGS) -DG_LOG_DOMAIN=\"dy\" -Isrc $(shell pkg-config --cflags $(APP_PKGS))
HOST_CFLAGS = $(CFLAGS) -DG_LOG_DOMAIN=\"dy-host\" -Ihost $(shell pkg-config --cflags $(HOST_PKGS))

APP_SRC  = $(shell find src -name '*.c')
HOST_SRC = $(shell find host -name '*.c')
APP_OBJ  = $(APP_SRC:%.c=build/%.o)
HOST_OBJ = $(HOST_SRC:%.c=build/%.o)

# Chromium-family browsers that get the native messaging manifest on install
BROWSER_DIRS = chromium google-chrome BraveSoftware/Brave-Browser vivaldi
NATIVE_HOST  = dev.soka.dy

all: dy dy-host compile_flags.txt

dy: $(APP_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(shell pkg-config --libs $(APP_PKGS))

dy-host: $(HOST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(shell pkg-config --libs $(HOST_PKGS))

build/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(APP_CFLAGS) -c -o $@ $<

build/host/%.o: host/%.c
	@mkdir -p $(dir $@)
	$(CC) $(HOST_CFLAGS) -c -o $@ $<

# for clangd
compile_flags.txt: Makefile
	printf '%s\n' -std=gnu23 -Wall -Wextra -Wno-unused-parameter -Isrc -Ihost \
		$(shell pkg-config --cflags $(APP_PKGS) $(HOST_PKGS)) > $@

install: dy dy-host
	install -Dm755 dy $(DESTDIR)$(PREFIX)/bin/dy
	install -Dm755 dy-host $(DESTDIR)$(PREFIX)/bin/dy-host
	install -Dm644 data/icons/dev.soka.dy.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/dev.soka.dy.svg
	install -d $(DESTDIR)$(PREFIX)/share/applications
	sed 's|@BINDIR@|$(PREFIX)/bin|' data/dev.soka.dy.desktop > $(DESTDIR)$(PREFIX)/share/applications/dev.soka.dy.desktop
	rm -rf $(DESTDIR)$(PREFIX)/share/dy/extension
	install -d $(DESTDIR)$(PREFIX)/share/dy
	cp -r extension $(DESTDIR)$(PREFIX)/share/dy/extension
	for b in $(BROWSER_DIRS); do \
		if [ "$$b" = chromium ] || [ -d "$(HOME)/.config/$$b" ]; then \
			install -d "$(HOME)/.config/$$b/NativeMessagingHosts"; \
			sed 's|@BINDIR@|$(PREFIX)/bin|' data/native-host.json > "$(HOME)/.config/$$b/NativeMessagingHosts/$(NATIVE_HOST).json"; \
		fi; \
	done
	@echo
	@echo "DY installed. To finish browser integration:"
	@echo "  chromium → chrome://extensions → Developer mode → Load unpacked → $(PREFIX)/share/dy/extension"

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dy $(DESTDIR)$(PREFIX)/bin/dy-host
	rm -f $(DESTDIR)$(PREFIX)/share/applications/dev.soka.dy.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/dev.soka.dy.svg
	rm -rf $(DESTDIR)$(PREFIX)/share/dy
	for b in $(BROWSER_DIRS); do rm -f "$(HOME)/.config/$$b/NativeMessagingHosts/$(NATIVE_HOST).json"; done

clean:
	rm -rf build dy dy-host

-include $(APP_OBJ:.o=.d) $(HOST_OBJ:.o=.d)

.PHONY: all install uninstall clean
