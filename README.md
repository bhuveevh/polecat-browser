# Polecat Browser

A modern, privacy-focused web browser for **Debian Linux**, built with **Qt 6
Widgets** and **Qt WebEngine**, and distributed **only** as a Debian `.deb`
package.

- **Developer:** OpentiaOS Developer
- **Email:** opentiaos@gmail.com
- **Website:** not provided
- **Application ID:** `org.opentiaos.PolecatBrowser`
- **Executable:** `polecat-browser`
- **Licence:** GPL-3.0-or-later

Polecat Browser is not a Chrome clone: it has its own visual identity while
staying familiar. It includes **no telemetry** and **no advertisements**, and it
does not send your browsing data anywhere unless you explicitly enable a feature
that requires it.

---

## Features

- Tabbed browsing and multiple windows, including **private windows**
- Address and search bar with automatic URL detection and HTTPS/security status
- **Bookmarks** with folders, import and export (HTML)
- **History** with search and clearing by time range
- **Download manager** with pause, resume, cancel, retry and safety warnings
- **Private browsing** using an off-the-record WebEngine profile (nothing is
  persisted)
- **GNOME-inspired interface**: header bar, navigation toolbar, tab strip,
  collapsible sidebar, light/dark/system theme
- **Privacy controls**: third-party cookie blocking, optional HTTPS-only mode,
  Do Not Track, per-site permissions, clear-browsing-data
- **Optional content blocking** from bundled filter lists with a user allowlist
- **Optional study tools**: reader mode, notes, vocabulary, reading list, focus
  timer and a Hindi–English translation helper
- Session save and restore, and reopen-closed-tab
- Full keyboard shortcuts, high-DPI aware, screen-reader-friendly labels

## Supported systems

- Debian 12 (bookworm) and newer; other Debian-based distributions should work.
- **GNOME 48, 49 and 50 are fully supported** (and older GNOME too). The browser
  is built on Qt Widgets and Qt WebEngine, not on GNOME-specific libraries, so
  it runs unchanged across GNOME releases. On a Wayland session, install
  `qt6-wayland` for a native Wayland experience (recommended, not required).
- Runs on any desktop environment, because it uses standard Qt Widgets rather
  than GNOME-only APIs.
- To build a package for a GNOME 48–50 machine, build it on the same Debian
  release as that machine (see `docs/BUILD.md`).

## Build dependencies

On Debian 12:

```bash
sudo apt install build-essential cmake \
    qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-tools-dev \
    libqt6sql6-sqlite libqt6svg6
# recommended on a Wayland session (GNOME 48-50):
sudo apt install qt6-wayland
```

Qt **6.4** or newer is required (this is the version in Debian 12). Newer Qt
works too; the code avoids APIs newer than 6.4 so older systems keep working.

## Building

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Run it directly:

```bash
./polecat-browser
```

## Building the Debian package

Two equivalent paths are provided.

**1. Quick local package with CPack:**

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
cpack -G DEB
```

**2. Native Debian packaging:**

```bash
# from the repository root
ln -s packaging/debian debian     # or copy the folder
dpkg-buildpackage -us -uc -b
```

The result is `polecat-browser_1.0.0_amd64.deb`.

## Installing and removing

```bash
sudo apt install ./polecat-browser_1.0.0_amd64.deb
# ...use the browser...
sudo apt remove polecat-browser
```

After installation Polecat Browser appears in the GNOME application menu, and
HTTP/HTTPS links can be opened with it once it is chosen as the default browser.

## Running the tests

```bash
cd build
cmake .. -DPOLECAT_BUILD_TESTS=ON
cmake --build . --parallel
ctest --output-on-failure
```

## Where your data lives

| Purpose       | Location                                   |
|---------------|--------------------------------------------|
| Configuration | `~/.config/polecat-browser/`               |
| Data          | `~/.local/share/polecat-browser/`          |
| Cache         | `~/.cache/polecat-browser/`                |
| Downloads     | your XDG download directory                |

Nothing is written under `/usr` or `/opt`, and no root privileges are needed at
runtime.

## Documentation

- `docs/QUICKSTART_DEB.md` — source archive to installed .deb, step by step
- `docs/ARCHITECTURE.md` — architecture and design decisions
- `docs/BUILD.md` — detailed build and packaging instructions
- `docs/DEVELOPER.md` — developer guide and code map
- `docs/USER_MANUAL.md` — user manual
- `docs/PRIVACY.md` — privacy statement
- `docs/KNOWN_LIMITATIONS.md` — known limitations
- `docs/ROADMAP.md` — future plans
- `CHANGELOG.md` — release history

## Licence

GPL-3.0-or-later. See `LICENSE` and `packaging/debian/copyright`.

Polecat Browser is not affiliated with Google, Mozilla, Microsoft or Apple.
