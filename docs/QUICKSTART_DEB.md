# From the source archive to an installed .deb

This is the shortest correct path from `polecat-browser-1.0.0.tar.gz` to a
browser that appears in your GNOME application menu. It works on Debian 12 and
newer, including the Debian releases that ship GNOME 48–50.

---

## 0. What you need first

You need a Debian machine (or a container/VM of one) and `sudo`. The package is
built *from source*, so you need the build tools and Qt 6 development packages.

```bash
sudo apt update
sudo apt install build-essential cmake \
    qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-tools-dev \
    libqt6sql6-sqlite libqt6svg6

# Recommended on a Wayland session (GNOME 48–50 default):
sudo apt install qt6-wayland
```

> **Which machine should build it?** Build on the **same Debian release as the
> machine you will install it on** (or an older one). A `.deb` built on bookworm
> (Qt 6.4) targets bookworm; a GNOME 48–50 machine on trixie/sid (Qt 6.8/6.9)
> should build on trixie/sid. The source is identical either way.

---

## 1. Unpack the archive

```bash
tar xzf polecat-browser-1.0.0.tar.gz
cd polecat-browser
```

You should see `CMakeLists.txt`, `src/`, `packaging/`, `resources/`, `tests/`
and `docs/`.

---

## 2. Build the .deb — pick ONE of the two routes

### Route A — CPack (fastest, good for local use)

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
cpack -G DEB
```

Result: a file named like **`polecat-browser-1.0.0-Linux.deb`** inside `build/`.

```bash
cd ..
```

### Route B — native Debian packaging (proper package metadata)

Debian tooling wants the `debian/` directory at the top of the tree, but the
packaging files live in `packaging/debian/`. Link them into place:

```bash
ln -s packaging/debian debian
dpkg-buildpackage -us -uc -b
```

Result: **`polecat-browser_1.0.0_amd64.deb`** in the **parent** directory (one
level up from `polecat-browser/`).

> If `dpkg-buildpackage` complains about missing `debhelper`, install it:
> `sudo apt install debhelper devscripts`.

---

## 3. Install it

```bash
# Route A output:
sudo apt install ./build/polecat-browser-1.0.0-Linux.deb

# Route B output (from the parent directory):
sudo apt install ./polecat-browser_1.0.0_amd64.deb
```

Using `apt install ./file.deb` (not `dpkg -i`) lets apt pull in any missing
dependencies automatically.

---

## 4. Run it

Launch **Polecat Browser** from the GNOME application menu, or:

```bash
polecat-browser
```

Check it works:

```bash
polecat-browser https://debian.org     # opens a page
polecat-browser --private              # private window
```

Set it as your default browser from **GNOME Settings → Default Applications**;
after that, clicking HTTP/HTTPS links opens Polecat Browser.

---

## 5. Verify and remove

```bash
dpkg -c build/polecat-browser-1.0.0-Linux.deb     # list packaged files
dpkg -I build/polecat-browser-1.0.0-Linux.deb     # read control metadata
dpkg -s polecat-browser                           # dependencies (after install)

sudo apt remove polecat-browser                   # clean uninstall
```

Removing the package leaves your bookmarks, history and settings in your home
directory untouched. Delete these by hand for a clean slate:

```bash
rm -rf ~/.config/polecat-browser ~/.local/share/polecat-browser ~/.cache/polecat-browser
```

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Qt6WebEngineWidgets ... not found` | install `qt6-webengine-dev` |
| `QSQLITE driver not loaded` at runtime | install `libqt6sql6-sqlite` |
| Icons appear blank | install `libqt6svg6` (SVG image plugin) |
| Blurry/odd scaling on Wayland | install `qt6-wayland` |
| Blank page or GPU error in a VM | Settings → Appearance → turn off hardware acceleration, or run with `QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu` |
| Refuses to start as root | run as a normal user; the WebEngine sandbox needs an unprivileged user |
| `dpkg-buildpackage: command not found` | `sudo apt install dpkg-dev` |

## Running the tests (optional)

```bash
cd build
cmake .. -DPOLECAT_BUILD_TESTS=ON
cmake --build . --parallel
ctest --output-on-failure
```
