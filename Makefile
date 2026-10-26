CC      = clang
PREFIX ?= $(HOME)/.local

APP_PKGS  = libadwaita-1 json-glib-1.0
HOST_PKGS = gio-2.0 gio-unix-2.0 json-glib-1.0

CFLAGS ?= -O2 -g
CFLAGS += -std=gnu23 -Wall -Wextra -Wno-unused-parameter -MMD -MP

APP_CFLAGS  = $(CFLAGS) -DG_LOG_DOMAIN=\"yoink\" -Isrc $(shell pkg-config --cflags $(APP_PKGS))
HOST_CFLAGS = $(CFLAGS) -DG_LOG_DOMAIN=\"yoink-host\" -Ihost $(shell pkg-config --cflags $(HOST_PKGS))

APP_SRC  = $(shell find src -name '*.c')
HOST_SRC = $(shell find host -name '*.c')
APP_OBJ  = $(APP_SRC:%.c=build/%.o)
HOST_OBJ = $(HOST_SRC:%.c=build/%.o)

# Chromium-family browsers that get the native messaging manifest on install
BROWSER_DIRS = chromium google-chrome BraveSoftware/Brave-Browser vivaldi
NATIVE_HOST  = dev.soka.yoink

all: yoink yoink-host compile_flags.txt

yoink: $(APP_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(shell pkg-config --libs $(APP_PKGS))

yoink-host: $(HOST_OBJ)
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

install: yoink yoink-host
	install -Dm755 yoink $(DESTDIR)$(PREFIX)/bin/yoink
	install -Dm755 yoink-host $(DESTDIR)$(PREFIX)/bin/yoink-host
	install -Dm644 data/icons/dev.soka.yoink.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/dev.soka.yoink.svg
	install -d $(DESTDIR)$(PREFIX)/share/applications
	sed 's|@BINDIR@|$(PREFIX)/bin|' data/dev.soka.yoink.desktop > $(DESTDIR)$(PREFIX)/share/applications/dev.soka.yoink.desktop
	rm -rf $(DESTDIR)$(PREFIX)/share/yoink/extension
	install -d $(DESTDIR)$(PREFIX)/share/yoink
	cp -r extension $(DESTDIR)$(PREFIX)/share/yoink/extension
	for b in $(BROWSER_DIRS); do \
		if [ "$$b" = chromium ] || [ -d "$(HOME)/.config/$$b" ]; then \
			install -d "$(HOME)/.config/$$b/NativeMessagingHosts"; \
			sed 's|@BINDIR@|$(PREFIX)/bin|' data/native-host.json > "$(HOME)/.config/$$b/NativeMessagingHosts/$(NATIVE_HOST).json"; \
		fi; \
	done
	@echo
	@echo "Yoink installed. To finish browser integration:"
	@echo "  chromium → chrome://extensions → Developer mode → Load unpacked → $(PREFIX)/share/yoink/extension"

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/yoink $(DESTDIR)$(PREFIX)/bin/yoink-host
	rm -f $(DESTDIR)$(PREFIX)/share/applications/dev.soka.yoink.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/dev.soka.yoink.svg
	rm -rf $(DESTDIR)$(PREFIX)/share/yoink/extension
	-rmdir $(DESTDIR)$(PREFIX)/share/yoink 2>/dev/null
	for b in $(BROWSER_DIRS); do rm -f "$(HOME)/.config/$$b/NativeMessagingHosts/$(NATIVE_HOST).json"; done

clean:
	rm -rf build yoink yoink-host

-include $(APP_OBJ:.o=.d) $(HOST_OBJ:.o=.d)

.PHONY: all install uninstall clean
