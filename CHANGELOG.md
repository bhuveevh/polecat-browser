# Changelog

All notable changes to Polecat Browser are documented here.
The format is based on *Keep a Changelog*, and this project adheres to
Semantic Versioning.

## [1.0.0] — 2026-01-01

### Added
- **Core browsing** (Phase 1): application window, Qt WebEngine view, address
  and search bar with automatic URL detection, back/forward, reload/stop, home.
- **Tabs and windows** (Phase 2): dedicated `TabManager`, tab strip with
  favicons, close buttons, drag reordering and pinned tabs; multiple windows;
  private windows; new-tab page; session save/restore and reopen-closed-tab.
- **Data** (Phase 3): SQLite database with versioned migrations; bookmarks with
  folders, search, HTML import/export; history with search and clearing by
  range. Corrupt-database recovery.
- **Downloads and private mode** (Phase 4): download manager with pause,
  resume, cancel and progress; safe filename handling (path-traversal defence);
  warnings for dangerous file types; private mode isolation via an
  off-the-record WebEngine profile.
- **GNOME-style UI** (Phase 5): header bar, navigation toolbar, tab strip,
  collapsible sidebar, find bar, light/dark/system theme, organised Settings
  window, About dialog with privacy statement and diagnostics.
- **Privacy and security** (Phase 5): third-party cookie blocking, optional
  HTTPS-only mode, Do Not Track, per-site permissions, clear-browsing-data,
  certificate-error prompts that are never silently bypassed.
- **Content blocking** (Phase 5): optional ad/tracker blocking from bundled
  filter lists with a user allowlist and per-site control.
- **Study tools** (Phase 5): reader mode, notes, vocabulary, reading list,
  focus timer, quick study links and a translation helper.
- **Packaging** (Phase 6): Debian `.deb` via CPack and native `dpkg-buildpackage`;
  desktop file, AppStream metainfo, hicolor icon, man page.
- **Testing** (Phase 7): Qt Test unit tests for URL handling, settings,
  database migrations, bookmarks and history.

### Security
- No telemetry, no advertisements, no silent extension installs, no silent
  changes to the default search engine or home page.
- Passwords are never stored in plain text.

### Known limitations
See `docs/KNOWN_LIMITATIONS.md`.
