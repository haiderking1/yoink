# Yoink

<img src="data/icons/dev.soka.yoink.svg" width="96" align="right" alt="Yoink icon">

A small, fast download manager for Linux. [aria2](https://aria2.github.io/) does
the downloading (16 connections per file, so it'll max out your line), and a
GTK4 + libadwaita window sits on top. Plain C, about 30 MB of RAM.

The browser extension works IDM style: every download Chromium starts gets
yoinked out of the browser and handed to Yoink, cookies and all. If Yoink
can't take it, the browser just downloads it like normal.

- Pause, resume and retry, and downloads survive restarts
- Asks where to save each download and remembers the last folder you used
- Links, magnets and .torrent files, from the window, drag and drop or the terminal
- Cancel cleans up the half-downloaded files, no leftovers
- Notifications when stuff finishes or fails

## Build and install

You need `aria2`, `gtk4`, `libadwaita` (1.5+), `json-glib`, `clang` (19+, for
`#embed`), `make` and `pkg-config`. On Arch:

```sh
sudo pacman -S aria2 gtk4 libadwaita json-glib clang make pkgconf
```

Then:

```sh
make
make install      # into ~/.local, no sudo needed
```

`make install` puts `yoink` and `yoink-host` in `~/.local/bin`, adds Yoink to
your app launcher, and sets up the bridge the browser extension talks to.

## Get the extension into Chrome

Works with Chromium, Google Chrome, Brave and Vivaldi.

1. Run `make install` first (above). It copies the extension to
   `~/.local/share/yoink/extension`.
2. Open `chrome://extensions` (on Brave it's `brave://extensions`).
3. Turn on **Developer mode** (switch in the top right).
4. Click **Load unpacked** and pick the `~/.local/share/yoink/extension`
   folder.
   `.local` is a hidden folder, so in the file picker hit **Ctrl+L** and paste
   the path, or **Ctrl+H** to show hidden folders.
5. That's it. Download anything and Yoink pops up asking where to save it.

Things to know:

- **Click the toolbar icon** to pause or resume catching downloads. The badge
  says `off` while it's paused.
- **Right-click a link → Yoink it** sends it to Yoink even while catching is
  paused.
- **Extension options** (right-click the icon → Options): minimum file size
  (smaller files stay in the browser, 1 MB by default), what to do when the
  size is unknown, and sites to never take downloads from.
- Downloads that come from a form submit (POST) stay in the browser, since
  they can't be replayed.
- If you install a browser after Yoink, run `make install` again so it gets
  the bridge too.

## Command line

```sh
yoink https://example.com/file.iso
yoink -H "Cookie: a=b" --referer https://example.com/ https://example.com/file.zip
yoink -o name.iso https://example.com/download?id=3
yoink some.torrent "magnet:?xt=urn:btih:…"
```

If Yoink is already open, the download goes to the open window.

## Uninstall

```sh
make uninstall
```

Then remove the extension from `chrome://extensions`. Your settings
(`~/.config/yoink`) and download queue (`~/.local/state/yoink`) are left
alone; delete them if you want a clean slate.

## License

MIT, see [LICENSE](LICENSE).
