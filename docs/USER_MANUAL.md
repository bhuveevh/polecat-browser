# Polecat Browser — user manual

## Getting started

Open **Polecat Browser** from your application menu, or run `polecat-browser`
in a terminal. You will see a new-tab page with a search box and a few study
links.

Type in the address bar:

- a web address, e.g. `debian.org` — the browser adds `https://` for you;
- or a search phrase, e.g. `qt webengine tutorial` — it searches with your
  chosen search engine.

Press **Enter** to go.

## The window at a glance

- **Header bar** — the page title, a **Private** badge in private windows, the
  profile button and the main menu (⋮).
- **Navigation toolbar** — Back, Forward, Reload/Stop, Home, the address bar,
  the bookmark star, the reader toggle and the menu.
- **Tab strip** — one tab per page, with a **+** button for a new tab.
- **Sidebar** — optional; holds Bookmarks, History, Downloads and Study Tools.
- **Content area** — the web page.

## Keyboard shortcuts

| Shortcut            | Action                       |
|---------------------|------------------------------|
| `Ctrl+L`            | Focus the address bar        |
| `Ctrl+T`            | New tab                      |
| `Ctrl+W`            | Close tab                    |
| `Ctrl+Shift+T`      | Reopen closed tab            |
| `Ctrl+N`            | New window                   |
| `Ctrl+Shift+P`      | New private window           |
| `Ctrl+R` / `F5`     | Reload                       |
| `Ctrl+F`            | Find in page                 |
| `Ctrl+D`            | Bookmark this page           |
| `Ctrl+H`            | History (opens the sidebar)  |
| `Ctrl+J`            | Downloads (opens the sidebar)|
| `Ctrl+B`            | Show/hide the sidebar        |
| `Ctrl+Plus` / `Ctrl+Minus` | Zoom in / out         |
| `Ctrl+0`            | Reset zoom                   |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Next / previous tab |
| `Alt+Left` / `Alt+Right` | Back / forward          |
| `F11`               | Full screen                  |
| `Ctrl+Shift+I`      | Developer tools              |
| `Ctrl+Q`            | Quit                         |

## Tabs

- Click **+** or press `Ctrl+T` for a new tab.
- Drag a tab to reorder it.
- Right-click a tab to reload, duplicate, pin, mute, or close it.
- **Pinned** tabs move to the front and stay there.
- A small speaker icon means a tab is playing or has been muted.
- Closing the last tab closes the window (you can change this in Settings).

## Bookmarks

- Press `Ctrl+D` or click the star to bookmark the current page. Click the star
  again to remove the bookmark.
- Open the sidebar's **Bookmarks** section to browse and search them.
- In Settings or the Bookmarks panel you can export your bookmarks to an HTML
  file and import bookmarks from another browser.

## History

Open **History** in the sidebar (or `Ctrl+H`). Type to search, double-click to
open, right-click to delete a single entry or clear all history.

History from **private windows is never saved**.

## Downloads

Downloads appear in the sidebar's **Downloads** section. For each download you
can:

- see its name, size, progress and speed;
- **pause**, **resume** or **cancel** it;
- **open** it or **open its containing folder** once finished;
- **remove** it from the list, or **clear completed** downloads.

Files with dangerous extensions (executables, scripts) show a warning before
they are opened. Polecat Browser **never runs a downloaded file automatically**.

## Private browsing

Open a private window with `Ctrl+Shift+P` or **New Private Window** from the
menu. In a private window:

- history, cookies, cache and form data are not saved on this device;
- downloads are not added to your download list;
- the window shows a **Private** badge.

**Private browsing does not make you anonymous online.** Your network, your
employer and the websites you visit may still be able to see your activity.
Bookmarks you add in a private window are still saved, because you asked for
them.

## Privacy settings

Open **Settings → Privacy** to:

- block third-party cookies;
- send *Do Not Track* requests;
- enable **HTTPS-only** mode;
- turn JavaScript, images and pop-ups on or off;
- control automatic media playback;
- enable **content blocking** (ads and trackers) and read where the filter
  lists come from;
- clear history, cookies, cache, downloads and permissions.

Permission requests (camera, microphone, location, notifications, clipboard)
are asked per site. You can remember a decision, and change it later in
Settings.

## Study tools

Open the sidebar's **Study Tools** section:

- **Notes** — write and save notes; export them as Markdown.
- **Vocabulary** — keep a list of words and meanings.
- **Reading** — save pages to read later.
- **Tools** — a focus timer, quick study links, and a translation helper.

The translation helper opens a translation website in a new tab when you click
**Translate**. Polecat Browser itself does not send your text anywhere.

## Themes

In **Settings → Appearance** choose **Follow system**, **Light** or **Dark**.
Polecat Browser never forces its brand colours over your system theme; the amber
and blue accents are used only for indicators and the address bar.

## Where your data is stored

- Configuration: `~/.config/polecat-browser/`
- Data (database, profiles, session): `~/.local/share/polecat-browser/`
- Cache: `~/.cache/polecat-browser/`
- Downloads: your usual Downloads folder

Uninstalling the package does not delete these; remove them by hand if you want
a clean slate.

## Getting help

Use **Main menu → About Polecat Browser**. The About dialog has a
**Copy diagnostic information** button that copies version, system and recent
log details — helpful when reporting a problem.
