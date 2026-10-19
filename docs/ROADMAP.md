# Roadmap

This is what we plan to work on next. Nothing here is a promise, and no feature
will ship with telemetry, ads, or silent network requests.

## Near term

- **Bookmarks bar** under the navigation toolbar, toggleable.
- **Vertical tabs** and **tab groups**.
- **Tab discarding / background-tab suspension** to reduce memory on low-end
  machines.
- **Session restore hardening**: periodic session snapshots so a crash loses at
  most a few seconds of state.
- **Search suggestions** with an explicit, per-engine opt-in and a clear
  explanation of what leaves the machine.

## Medium term

- **Keyring-backed password manager** using the freedesktop Secret Service
  (GNOME Keyring), with an option to disable it entirely.
- **Per-site settings** UI (cookies, JavaScript, blocking) reachable from the
  address bar's security popover.
- **Richer certificate viewer** with a readable explanation of each field.
- **Filter-list management**: view sources and update dates, add custom lists,
  and a clearer per-site blocking toggle.
- **Full-text history search** with better ranking and grouping.
- **Reader mode v2** using a proper article extractor.

## Longer term

- **Highlights** that persist with the page URL, and a highlights view.
- **Notes linked to pages**, with Markdown export per page.
- **Optional, user-driven sync** of bookmarks only, using a self-hosted endpoint
  the user configures — never a service we run by default.
- **Improved accessibility** pass: a full keyboard-navigation audit and
  screen-reader testing on GNOME.
- **Broader desktop testing**: KDE Plasma and XFCE on Debian.

## Explicitly out of scope

- AppImage, Flatpak, Snap, Windows, macOS or Android builds.
- Any form of telemetry, advertising or sponsored content.
- Any feature that sends browsing data to a remote server without the user
  turning it on and knowing what it does.
