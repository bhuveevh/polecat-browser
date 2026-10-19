# Privacy statement

Polecat Browser is designed so that your browsing stays yours.

## What we do not do

- **No telemetry.** The browser does not report usage, crashes or statistics to
  any server we control.
- **No advertising.** There are no ads, sponsored links or promoted tiles.
- **No silent network requests for your data.** URLs and page content are not
  sent anywhere unless you explicitly use a feature that requires it.
- **No silent extension installs.**
- **No silent changes** to your default search engine or home page.
- **No plain-text password storage.** Polecat Browser does not implement its own
  password store; it relies on your system keyring if you use one.

## What is stored, and where

Everything Polecat Browser remembers is stored locally, under your home
directory, and only for the features you use:

- **Bookmarks** — in `~/.local/share/polecat-browser/polecat.db`.
- **History** — the same database. You can clear it at any time.
- **Downloads** — metadata in the database; files where you choose to save them.
- **Site permissions** — the same database, so your decisions are reversible.
- **Notes, vocabulary, reading list** — the same database (only if you use the
  study tools).
- **Cookies, cache, local storage** — in the per-profile directories under
  `~/.local/share/polecat-browser/profiles/` and
  `~/.cache/polecat-browser/`.

## Private browsing

Private windows use an **off-the-record** WebEngine profile. Nothing from a
private window — history, cookies, cache, form data or download list entries —
is written to disk, and it disappears when the last private window closes.

Private browsing hides your activity **from this device only**. It does not make
you anonymous online: your network operator, your employer, and the websites you
visit may still be able to see what you do.

## Content blocking

If you enable content blocking, filter lists are read from disk (bundled with
the application, plus an optional list you create at
`~/.config/polecat-browser/filters/user.txt`). They are **not downloaded without
your knowledge**, and the source and update date of the bundled list are shown
in Privacy settings. Blocking is best-effort and is **not** a guarantee of
privacy.

## Third parties

Polecat Browser is built on Qt and Qt WebEngine, which embeds the Chromium
project. Those components may make requests required to render the pages you
visit (for example, fetching the page and its resources). That is ordinary
browsing, driven by the sites you open, not by us.

If you enable search suggestions, what you type is sent to your search engine.
Leave suggestions off for maximum privacy.

## Your control

- **Settings → Privacy** lets you clear history, cookies, cache, downloads and
  permissions, and change every privacy-related option.
- Removing the package leaves your data in place; delete the folders above if
  you want them gone.
