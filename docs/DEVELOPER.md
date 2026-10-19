# Developer guide

## Layout in one paragraph

`Application` (an `QApplication` subclass) owns every application-wide service
and the set of open windows. Each `BrowserWindow` owns a `TabManager`, which
owns `BrowserTab`s, each of which owns a `WebView`/`WebPage`. Managers never
reach across to each other except through the `AppContext` bundle. See
`docs/ARCHITECTURE.md` for the full picture.

## Code map

| Area              | Files                                                |
|-------------------|------------------------------------------------------|
| Entry point       | `src/main.cpp`                                       |
| Application       | `src/app/Application.{h,cpp}`, `src/app/AppContext.h`|
| Window            | `src/browser/BrowserWindow.{h,cpp}`                  |
| Web engine        | `src/browser/WebView.{h,cpp}`, `WebPage.{h,cpp}`     |
| Chrome            | `src/browser/NavigationToolBar.*`, `AddressBar.*`    |
| Tabs              | `src/tabs/TabManager.*`, `BrowserTab.*`, `TabBar.*`  |
| Data              | `src/database/DatabaseManager.*`                     |
| Bookmarks         | `src/bookmarks/BookmarkManager.*`, `BookmarksPanel.*`|
| History           | `src/history/HistoryManager.*`, `HistoryPanel.*`     |
| Downloads         | `src/downloads/DownloadsManager.*`, `DownloadsPanel.*`|
| Profiles          | `src/profiles/ProfileManager.*`                      |
| Privacy           | `src/privacy/PrivacyManager.*`, `ContentBlocker.*`, `PermissionManager.*` |
| Settings          | `src/settings/SettingsManager.*`, `SettingsDialog.*` |
| Study             | `src/study/StudyPanel.*`                             |
| UI helpers        | `src/ui/HeaderBar.*`, `Sidebar.*`, `FindBar.*`, `AboutDialog.*`, `ThemeManager.*` |
| Utilities         | `src/utils/Paths.*`, `UrlUtils.*`, `Logging.*`, `IconLoader.*` |

## Conventions

- **C++17**, four-space indent, `Polecat` namespace, `m_` prefix for members.
- All user-facing strings go through `tr()` so they can be translated. Never
  hard-code English in a widget.
- All SQL uses prepared statements (`QSqlQuery::prepare` + `addBindValue`).
  Multi-statement operations run inside a transaction.
- Never construct a `QWebEngineProfile` outside `ProfileManager`.
- Never write to disk outside the XDG locations in `Paths`.
- Log with the category that fits: `lcApp`, `lcDb`, `lcNet`, `lcUi`, `lcPrivacy`.
  Enable them at runtime with `QT_LOGGING_RULES="polecat.*=true"`.

## Adding a database migration

1. Append a new `Migration{version, {statements...}}` to `migrations()` in
   `DatabaseManager.cpp`. Never edit a shipped migration.
2. Keep every statement idempotent-friendly (`CREATE TABLE IF NOT EXISTS`,
   `CREATE INDEX IF NOT EXISTS`).
3. Add a test to `tests/tst_database.cpp` if the migration adds structure.

## Adding a setting

1. Add a key constant and a default in `SettingsManager`.
2. Read it where needed through `value(key, default)`.
3. Add a control in the relevant `SettingsDialog` section, using the
   `boundCheck` / `boundCombo` helpers.
4. React to changes by connecting to `SettingsManager::settingChanged`.

## Adding a toolbar action or menu item

Add it in `BrowserWindow::buildMenus()` (main menu) or
`NavigationToolBar`'s constructor (toolbar), give it an accessible name and a
tooltip, and connect it to a slot. Add a `QShortcut` in
`BrowserWindow::buildShortcuts()` if it has a keyboard shortcut.

## Threading and performance

- The UI runs on the main thread; SQLite work is quick and runs there too, but
  keep queries indexed and bounded (`LIMIT`).
- Tabs are created lazily and views are freed when a tab closes.
- Avoid creating extra `QWebEngineProfile`s; reuse the ones `ProfileManager`
  hands out.
- Do not block the UI thread on network or disk.

## Tests

Unit tests use Qt Test. Tests that touch the filesystem call
`QStandardPaths::setTestModeEnabled(true)` so they never read or write the
user's real configuration. Run them with `ctest` from the build directory.

## Manual test checklist (before a release)

1. Launch from the GNOME application menu.
2. Open an `https://` site; check the padlock and certificate info.
3. Open `http://` sites with HTTPS-only on and off.
4. Open many tabs; drag to reorder; pin one; mute one; close and reopen.
5. Open a private window; confirm the badge and that nothing appears in history
   afterwards.
6. Add, edit and delete bookmarks; export and re-import them.
7. Search history; clear a time range.
8. Download a file; pause, resume, cancel; try a file that already exists.
9. Resize to a narrow window; confirm the sidebar hides and layout adapts.
10. Switch theme light/dark/system.
11. Restart the browser; confirm the session is restored.
12. Check High-DPI scaling on a HiDPI display if available.
