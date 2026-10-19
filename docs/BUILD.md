# Building and packaging Polecat Browser

## 1. Prerequisites

Polecat Browser targets **Debian 12 (bookworm)** and newer. It needs Qt 6.4 or
newer with the WebEngine module.

```bash
sudo apt update
sudo apt install build-essential cmake \
    qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-tools-dev \
    libqt6sql6-sqlite libqt6svg6
```

For a **native Wayland session under GNOME 48–50**, also install the Wayland
platform plugin (it is only recommended at runtime, so the browser still works
via XWayland without it):

```bash
sudo apt install qt6-wayland
```

To check the Qt version that will be used:

```bash
qmake6 --version        # or: dpkg -s qt6-base-dev | grep Version
```

> **Building for GNOME 48–50:** the browser needs no code changes, but build the
> `.deb` on the **same Debian release as the target machine** (e.g. trixie/sid,
> which ships Qt 6.8/6.9) so the Qt symbol versions match. A package built on
> bookworm (Qt 6.4) is meant for bookworm.

## 2. A normal build

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

Useful CMake options:

| Option                    | Default | Meaning                          |
|---------------------------|---------|----------------------------------|
| `CMAKE_BUILD_TYPE`        | Release | `Release`, `Debug`, `RelWithDebInfo` |
| `POLECAT_BUILD_TESTS`     | ON      | Build the Qt Test unit tests     |
| `CMAKE_INSTALL_PREFIX`    | `/usr/local` | Install prefix              |

## 3. Running

```bash
./polecat-browser                 # new-tab page
./polecat-browser https://debian.org
./polecat-browser --private       # private window
./polecat-browser --profile work  # named profile
```

## 4. Unit tests

```bash
cd build
cmake .. -DPOLECAT_BUILD_TESTS=ON
cmake --build . --parallel
ctest --output-on-failure
```

Individual tests can be run directly, e.g. `./tests/tst_urlutils`.

## 5. Building the .deb

### 5a. CPack (quick)

```bash
cd build
cpack -G DEB
# -> polecat-browser-1.0.0-Linux.deb
```

### 5b. Native Debian packaging (recommended for release)

The packaging files live in `packaging/debian/`. Debian tooling expects them at
the repository root, so either symlink or copy:

```bash
ln -s packaging/debian debian
dpkg-buildpackage -us -uc -b
```

This produces `../polecat-browser_1.0.0_amd64.deb`.

Run packaging lint:

```bash
lintian ../polecat-browser_1.0.0_amd64.deb
```

## 6. Installing and removing

```bash
sudo apt install ./polecat-browser_1.0.0_amd64.deb
# launch from the GNOME application menu, or:
polecat-browser
sudo apt remove polecat-browser
```

Removing the package leaves your bookmarks, history and settings under your home
directory untouched, by design.

## 7. Verifying the package contents

```bash
dpkg -c polecat-browser_1.0.0_amd64.deb          # file list
dpkg -I polecat-browser_1.0.0_amd64.deb          # control metadata
dpkg -s polecat-browser                          # after install: dependencies
```

Expected installed files:

```
/usr/bin/polecat-browser
/usr/share/applications/org.opentiaos.PolecatBrowser.desktop
/usr/share/metainfo/org.opentiaos.PolecatBrowser.metainfo.xml
/usr/share/icons/hicolor/scalable/apps/org.opentiaos.PolecatBrowser.svg
/usr/share/man/man1/polecat-browser.1
/usr/share/doc/polecat-browser/...
```

## 8. Troubleshooting the build

- **`Qt6WebEngineWidgets` not found** — install `qt6-webengine-dev`.
- **`QSQLITE` driver missing at runtime** — install `libqt6sql6-sqlite`.
- **Blank web views / GPU errors on a VM** — turn off hardware acceleration in
  Settings → Appearance, or run with `QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu`.
- **Sandbox errors as root** — do not run the browser as root; WebEngine's
  sandbox requires an unprivileged user.
