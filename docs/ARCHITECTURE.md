# Polecat Browser — Architecture

**Polecat Browser** is a modern, privacy-focused desktop web browser for
Debian-based Linux, built with **Qt 6 Widgets** and **Qt WebEngine**, and
distributed **only** as a Debian `.deb` package.

- Developer: OpentiaOS Developer
- Contact: opentiaos@gmail.com
- Application ID: `org.opentiaos.PolecatBrowser`
- Executable: `polecat-browser`

---

## 1. Technology stack

| Concern            | Choice                                             |
|--------------------|----------------------------------------------------|
| Language           | C++17 (C++20 where available)                      |
| UI toolkit         | Qt 6 Widgets (QML only where it clearly helps)     |
| Web engine         | Qt WebEngine (Chromium-based)                      |
| Build system       | CMake ≥ 3.16                                       |
| Browser data       | SQLite (via `Qt6::Sql`, driver `QSQLITE`)          |
| Settings           | QSettings (INI, XDG config dir)                    |
| Icons              | Bundled SVG + Qt standard icons                    |
| Packaging          | Debian `.deb` (CPack `DEB` generator and/or `dpkg-buildpackage`) |
| Tests              | Qt Test (`Qt6::Test`)                              |
| Logging            | Qt logging categories (`QLoggingCategory`)         |

We deliberately keep the UI in **Widgets** rather than QML: Qt WebEngine's
`QWebEngineView` is a Widget, Widgets give us native GNOME-adjacent window
chrome, high-DPI handling and accessibility for free, and the tree stays easy
to build and audit.

## 2. Process and object model

A single process with a single `QApplication`. The object graph is a tree
rooted at `Application`:

```
Application (QApplication subclass)
├── Paths / Logging / SettingsManager        (singletons, no UI)
├── DatabaseManager                          (SQLite, thread-affine)
├── ProfileManager                           (owns QWebEngineProfile objects)
├── DownloadsManager                         (app-wide, per-profile)
├── BookmarkManager / HistoryManager         (wrap DatabaseManager)
├── ContentBlocker (QWebEngineUrlRequestInterceptor)
└── BrowserWindow (0..n, top-level QMainWindow)
    └── TabManager
        └── BrowserTab (0..n)
            ├── WebView   (QWebEngineView subclass)
            └── WebPage   (QWebEnginePage subclass — new-window/permission/cert policy)
```

Everything above the window level is a **singleton owned by `Application`**
and torn down in reverse order at exit. This gives us one database connection,
one set of managers, and many windows — exactly how a desktop browser behaves.

### Why these seams

- **`Application`** — owns lifetime, applies theme, wires global shortcuts,
  parses CLI arguments (`URL`, `--private`, `--new-window`, `--profile=`).
- **`ProfileManager`** — the *only* place `QWebEngineProfile` is constructed.
  Nothing else may create a profile; this prevents the classic WebEngine bug
  of accidentally spawning many profiles and multiplying memory/cache.
- **`TabManager`** — pure tab lifecycle logic, no windowing. It emits signals
  (`tabCreated`, `currentChanged`, `tabClosed`) that the window chrome listens
  to. This is what makes tabs unit-testable without a visible window.
- **`WebPage`** — a subclass that centralises policy: new-window requests,
  permission prompts, certificate errors, full-screen requests, download
  interception. Keeping policy here means `BrowserTab` stays thin.
- **`DatabaseManager`** — owns one `QSqlDatabase` connection and runs
  versioned migrations. Managers (`BookmarkManager`, `HistoryManager`,
  `DownloadsManager`) never touch SQL directly outside their own class.

## 3. Qt WebEngine profile strategy

This is the security-critical part, so it is spelled out precisely.

`ProfileManager` creates and owns every profile:

- **Persistent profile** (`"default"`, and one per named profile): a named
  `QWebEngineProfile("default")` whose
  `setPersistentStoragePath()` → `~/.local/share/polecat-browser/profiles/<name>/`
  and `setCachePath()` → `~/.cache/polecat-browser/profiles/<name>/cache/`.
  `setPersistentCookiesPolicy(ForcePersistentCookies)`.
- **Private profile** (off-the-record): a named `QWebEngineProfile` **with an
  empty storage name**, which Qt WebEngine treats as *off-the-record*. Its
  cache type is `MemoryHttpCache` and cookies are `NoPersistentCookies`.
  Nothing is written to disk. One private profile is shared by all private
  windows in a session and destroyed when the last private window closes.

Only profiles created through `ProfileManager` are ever used. `BrowserTab`
receives a profile pointer; it never constructs one.

### Private-mode isolation guarantees

- No history row is written: `HistoryManager::recordVisit()` early-returns when
  `profile->isOffTheRecord()` is true.
- No download metadata is persisted for off-the-record downloads.
- Cookies/cache/form data live only in memory and vanish at profile teardown.
- The window title and a persistent header-bar badge show **Private**.
- We do **not** claim anonymity. The in-UI privacy note says plainly that
  private mode hides local activity only; the network and the sites you visit
  can still see you.

## 4. Data model (SQLite)

One database: `~/.local/share/polecat-browser/polecat.db`.

Schema is versioned through `PRAGMA user_version` and a migration list. Every
migration is applied inside a transaction; a failure rolls back and leaves the
version untouched (database error recovery).

Tables:

- `bookmarks(id, parent_id, title, url, favicon, created, modified, position)`
- `bookmark_folders` — modelled as `bookmarks` rows with `url IS NULL` and
  `parent_id` forming the tree.
- `history(id, title, url, visit_time, visit_count, last_visit, title_norm, url_norm)`
- `downloads(id, file_name, source_url, destination, total_bytes, received_bytes, mime, start_time, end_time, state)`
- `permissions(host, feature, decision, updated)`
- `notes(id, url, title, body, created, modified)`
- `vocabulary(id, word, meaning, language, created)`

Indexes on `history(url_norm)`, `history(last_visit)`, `bookmarks(parent_id)`,
`permissions(host, feature)`.

All queries use prepared statements and, where more than one statement is
involved, an explicit transaction. Normalised columns (`*_norm`) are
lower-cased, whitespace-collapsed copies used for `LIKE` search.

## 5. Security model

- **Transport**: HTTPS state is shown per-tab; certificate errors are surfaced
  through `WebPage::certificateError`. We never silently bypass an error — the
  user is asked and the decision is recorded per-host in `permissions`.
- **Permissions** (camera, mic, location, notifications, clipboard, full-screen):
  mediated by `PermissionManager`, prompted per origin, stored so a decision is
  reversible and re-askable.
- **Downloads**: filenames are sanitised (path-traversal and control chars
  stripped), dangerous extensions are flagged before opening, and files are
  never auto-executed.
- **Content blocking**: `ContentBlocker` is a `QWebEngineUrlRequestInterceptor`
  applying bundled filter lists plus a user allowlist. It is optional and
  off-by-default per spec wording is respected — the user enables it.
- **Passwords**: no plaintext storage. A keyring-backed password manager is a
  future item; until then the browser relies on the user's own keyring and does
  not implement unsafe storage.
- **No telemetry**, no ads, no silent default-search/home-page changes, no
  silent extension installs.

## 6. UI composition (GNOME-inspired)

`BrowserWindow` is a `QMainWindow` with a custom header bar:

1. **Header bar** — window title / tab title, profile chip, private badge,
   main menu button (`⋮`). Uses client-side decoration when the platform allows
   it, but degrades to a normal title bar otherwise (no hard dependency on
   GNOME 48 CSD APIs).
2. **Navigation toolbar** — Back, Forward, Reload/Stop, address/search field,
   bookmark star, reader toggle, menu.
3. **Tab bar** — `QTabBar` subclass with new-tab button, favicon, close button,
   drag reorder, pinned tabs, context menu.
4. **Optional sidebar** — collapsible; Bookmarks / History / Downloads /
   Reading List / Notes / Study Tools. Overlays on narrow windows.
5. **Web content area** — the current `WebView`.
6. **Find bar** and **status/info strip** as transient overlays.

Themes follow the system by default; explicit light/dark is available. Accent
colours (`#D88A24` amber, `#3D78B8` blue) are used only for branding, never
forced over a user theme.

## 7. Debian compatibility

- Built and tested against **Debian 12 (bookworm)** and Qt 6.4, which is the
  version in bookworm. No GNOME-48-only API is used, so older GNOME works.
- `CMakeLists.txt` declares a minimum Qt of 6.4 and degrades gracefully if
  WebEngine Widgets is present but a newer helper API is not.
- Dependencies are expressed as Debian package names in
  `packaging/debian/control` (`libqt6webenginewidgets6`, `libqt6sql6-sqlite`,
  `libqt6svg6`, etc.).

### GNOME 48, 49 and 50

Polecat Browser is written against **Qt Widgets and Qt WebEngine only** — it
does not link against libadwaita, GTK, or any version-specific GNOME library.
That is a deliberate choice, and it is what makes the browser portable across
GNOME releases:

- **No GNOME-48-only APIs.** Nothing in the code calls a GNOME 48 addition, so
  GNOME 43 through GNOME 50 behave the same. On newer GNOME the desktop simply
  supplies newer Qt libraries underneath.
- **Wayland and X11.** GNOME 48–50 default to a Wayland session. Qt 6 handles
  Wayland natively; the packaging *recommends* `qt6-wayland` so the app uses the
  native Wayland platform plugin instead of falling back to XWayland. On X11
  sessions nothing extra is needed.
- **Dark mode.** `ThemeManager` reads the platform colour scheme through
  `QStyleHints::colorScheme()` on Qt 6.5+ (the version shipped by the Debian
  releases that carry GNOME 48–50), and falls back to palette inspection on
  older Qt. So "Follow system" tracks GNOME's dark/light preference either way.
- **Client-side decorations.** The custom header bar is an ordinary Qt widget,
  so it works under GNOME's CSD and under X11/other desktops without special
  casing. We never assume a specific window-manager protocol.
- **Build on the target distro.** A `.deb` built against Qt 6.4 (bookworm)
  installs cleanly on bookworm. For a GNOME 48–50 machine running Debian
  trixie/sid (Qt 6.8/6.9), build the package on that same distro — or one no
  newer than it — so the Qt symbol versions match. The source needs no changes;
  only the build host changes.
- Two packaging paths are provided and documented:
  1. **CPack** — `cmake --build . && cpack -G DEB` for a quick local `.deb`.
  2. **Native Debian** — `packaging/debian/*` with `dpkg-buildpackage -us -uc -b`.
- XDG paths only; nothing is written under `/usr` or `/opt` at runtime.

## 8. File structure

```
polecat-browser/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── CHANGELOG.md
├── .gitignore
├── docs/
│   ├── ARCHITECTURE.md        (this file)
│   ├── BUILD.md
│   ├── DEVELOPER.md
│   ├── USER_MANUAL.md
│   ├── PRIVACY.md
│   ├── KNOWN_LIMITATIONS.md
│   └── ROADMAP.md
├── packaging/
│   ├── debian/
│   │   ├── control
│   │   ├── rules
│   │   ├── changelog
│   │   ├── copyright
│   │   ├── install
│   │   ├── postinst
│   │   ├── prerm
│   │   ├── postrm
│   │   ├── source/format
│   │   └── org.opentiaos.PolecatBrowser.desktop
│   └── org.opentiaos.PolecatBrowser.metainfo.xml
├── resources/
│   ├── icons/org.opentiaos.PolecatBrowser.svg
│   ├── icons/polecat-*.svg            (toolbar glyphs)
│   ├── translations/polecat_en.ts
│   ├── translations/polecat_hi.ts
│   ├── html/newtab.html
│   ├── html/error.html
│   ├── html/privatenote.html
│   └── filters/easylist-sample.txt
├── src/
│   ├── main.cpp
│   ├── app/Application.{h,cpp}
│   ├── browser/BrowserWindow.{h,cpp}
│   ├── browser/WebView.{h,cpp}
│   ├── browser/WebPage.{h,cpp}
│   ├── browser/NavigationToolBar.{h,cpp}
│   ├── browser/AddressBar.{h,cpp}
│   ├── tabs/TabManager.{h,cpp}
│   ├── tabs/BrowserTab.{h,cpp}
│   ├── tabs/TabBar.{h,cpp}
│   ├── downloads/DownloadsManager.{h,cpp}
│   ├── downloads/DownloadsPanel.{h,cpp}
│   ├── bookmarks/BookmarkManager.{h,cpp}
│   ├── bookmarks/BookmarksPanel.{h,cpp}
│   ├── history/HistoryManager.{h,cpp}
│   ├── history/HistoryPanel.{h,cpp}
│   ├── privacy/PrivacyManager.{h,cpp}
│   ├── privacy/ContentBlocker.{h,cpp}
│   ├── privacy/PermissionManager.{h,cpp}
│   ├── settings/SettingsManager.{h,cpp}
│   ├── settings/SettingsDialog.{h,cpp}
│   ├── profiles/ProfileManager.{h,cpp}
│   ├── ui/HeaderBar.{h,cpp}
│   ├── ui/Sidebar.{h,cpp}
│   ├── ui/FindBar.{h,cpp}
│   ├── ui/AboutDialog.{h,cpp}
│   ├── ui/ThemeManager.{h,cpp}
│   ├── database/DatabaseManager.{h,cpp}
│   ├── study/StudyPanel.{h,cpp}
│   └── utils/Paths.{h,cpp}
│       utils/UrlUtils.{h,cpp}
│       utils/Logging.{h,cpp}
│       utils/IconLoader.{h,cpp}
└── tests/
    ├── CMakeLists.txt
    ├── tst_urlutils.cpp
    ├── tst_database.cpp
    ├── tst_bookmarks.cpp
    ├── tst_history.cpp
    └── tst_settings.cpp
```

## 9. Build & packaging flow

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
cpack -G DEB                       # quick local .deb
# — or native —
dpkg-buildpackage -us -uc -b       # from packaging/debian
sudo apt install ./polecat-browser_1.0.0_amd64.deb
sudo apt remove polecat-browser
```

## 10. Development phases

The build follows the seven phases in the specification (window+view → tabs →
data → GNOME UI → privacy → study tools → packaging/testing). Each module above
carries the phase it was introduced in, and no module depends on a later phase.
