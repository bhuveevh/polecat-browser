# Known limitations

Polecat Browser 1.0.0 is an honest, working browser, but it is deliberately
scoped. These are the things it does **not** do yet, or does in a simple way.

## Browsing

- **No browser extensions / add-ons.** There is no extension API or store, and
  none are installed silently.
- **No built-in password manager.** Passwords are never stored in plain text;
  the browser does not yet integrate with a system keyring, so it simply does
  not offer to save passwords.
- **No bookmarks bar under the toolbar.** Bookmarks live in the sidebar and in
  the menu. A horizontal bookmarks bar is planned.
- **No sync.** Bookmarks, history and settings stay on one machine.
- **No form autofill.**
- **Reader mode is basic.** It renders the page's visible text with comfortable
  typography; it is not a full article extractor, so it can include navigation
  text or miss some content on complex pages.
- **View source** shows the page's current DOM as text; it is not the raw
  network bytes.

## Privacy and security

- **Content blocking is best-effort.** It uses a small bundled filter list and a
  simple matcher. It does not implement the full syntax of large community
  lists, and it does not claim perfect blocking.
- **No per-site cookie exceptions UI** beyond the global third-party cookie
  setting.
- **Referrer and WebRTC controls** are limited to what Qt WebEngine exposes;
  they are applied where the engine allows and are not a complete privacy
  solution.
- **HTTPS-only mode** upgrades main-frame `http` navigations to `https`; it does
  not rewrite every subresource.
- **Certificate overrides** are per-session prompts; there is no permanent
  exception list yet.

## Tabs and windows

- **No tab groups** and **no vertical tabs** yet (both are on the roadmap).
- **Tab bar position** is stored as a setting but the layout currently keeps the
  tab bar at the top.
- **Background-tab suspension** is not implemented; many open tabs use more
  memory than a browser with tab discarding.

## Study tools

- **The dictionary** does not look words up itself; it opens your chosen
  dictionary site or adds the word to your vocabulary list.
- **Translation** opens a translation website in a tab rather than translating
  in-process, so that no data leaves your machine without your action.
- **Highlights** on pages are not persisted.

## Platform

- **Debian only.** There are no AppImage, Flatpak, Snap, Windows, macOS or
  Android builds, by design.
- **Tested primarily on GNOME.** It runs on other desktops (it uses standard Qt
  Widgets), but the interface is tuned for GNOME.
- **Qt 6.4 or newer** is required. On systems with only Qt 5, the browser will
  not build.
- **Running as root is unsupported** because Qt WebEngine's sandbox needs an
  unprivileged user.

## Testing

The included unit tests cover URL handling, settings, database migrations,
bookmarks and history. The interactive UI (tabs, downloads, themes, session
restore) is covered by the manual test checklist in `docs/DEVELOPER.md` rather
than by automated UI tests.
