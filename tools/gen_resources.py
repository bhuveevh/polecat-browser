#!/usr/bin/env python3
"""Generates Polecat Browser's bundled resources (icons, HTML, filters).

Run from the repository root:  python3 tools/gen_resources.py
Everything it writes lives under resources/.
"""
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
RES = os.path.join(ROOT, "resources")


def write(path, content):
    full = os.path.join(ROOT, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, "w", encoding="utf-8") as f:
        f.write(content)
    print("wrote", path)


# ---------------------------------------------------------------------------
# Brand icon: an abstract polecat mark. Dark charcoal plate, amber ears/mask,
# cool-blue eyes. Not derived from any existing browser logo.
# ---------------------------------------------------------------------------
write("resources/icons/org.opentiaos.PolecatBrowser.svg", """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">
  <defs>
    <linearGradient id="plate" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#2b2b2c"/>
      <stop offset="1" stop-color="#1a1a1b"/>
    </linearGradient>
  </defs>
  <rect x="16" y="16" width="224" height="224" rx="52" fill="url(#plate)"/>
  <!-- ears -->
  <path d="M70 96 L86 48 L116 84 Z" fill="#D88A24"/>
  <path d="M186 96 L170 48 L140 84 Z" fill="#D88A24"/>
  <!-- face mask -->
  <path d="M128 66 C170 66 194 96 194 134 C194 178 166 208 128 208
           C90 208 62 178 62 134 C62 96 86 66 128 66 Z" fill="#D88A24"/>
  <!-- muzzle -->
  <path d="M128 132 C150 132 164 150 164 170 C164 190 148 202 128 202
           C108 202 92 190 92 170 C92 150 106 132 128 132 Z" fill="#F5E3C8"/>
  <!-- eyes -->
  <circle cx="104" cy="128" r="11" fill="#3D78B8"/>
  <circle cx="152" cy="128" r="11" fill="#3D78B8"/>
  <circle cx="104" cy="128" r="4" fill="#101010"/>
  <circle cx="152" cy="128" r="4" fill="#101010"/>
  <!-- nose -->
  <path d="M128 158 L138 168 L128 178 L118 168 Z" fill="#202124"/>
</svg>
""")

# ---------------------------------------------------------------------------
# Toolbar glyphs. Simple, legible at 16px, neutral grey so they read on both
# light and dark themes. On GNOME the themed icon is preferred; these are the
# fallback shipped with the app.
# ---------------------------------------------------------------------------
GREY = "#8a8a8a"
STROKE = ('fill="none" stroke="%s" stroke-width="2" '
          'stroke-linecap="round" stroke-linejoin="round"' % GREY)


def glyph(name, body):
    write("resources/icons/polecat-%s.svg" % name,
          '<?xml version="1.0" encoding="UTF-8"?>\n'
          '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" '
          'viewBox="0 0 24 24">\n%s\n</svg>\n' % body)


glyph("back", '  <path d="M15 4 L7 12 L15 20" %s/>' % STROKE)
glyph("forward", '  <path d="M9 4 L17 12 L9 20" %s/>' % STROKE)
glyph("reload", '  <path d="M20 12a8 8 0 1 1-2.3-5.6" %s/>\n'
                '  <path d="M20 4 v4 h-4" %s/>' % (STROKE, STROKE))
glyph("stop", '  <path d="M6 6 L18 18" %s/>\n  <path d="M18 6 L6 18" %s/>' % (STROKE, STROKE))
glyph("home", '  <path d="M4 11 L12 4 L20 11" %s/>\n'
              '  <path d="M6 10 V20 H18 V10" %s/>' % (STROKE, STROKE))
glyph("new-tab", '  <path d="M12 5 V19" %s/>\n  <path d="M5 12 H19" %s/>' % (STROKE, STROKE))
glyph("close", '  <path d="M6 6 L18 18" %s/>\n  <path d="M18 6 L6 18" %s/>' % (STROKE, STROKE))
glyph("bookmark", '  <path d="M7 3 H17 V21 L12 16 L7 21 Z" %s/>' % STROKE)
glyph("bookmarks", '  <path d="M7 3 H15 V21 L11 17 L7 21 Z" %s/>\n'
                   '  <path d="M10 3 H18 V21 L14 17" %s/>' % (STROKE, STROKE))
glyph("history", '  <circle cx="12" cy="12" r="8" %s/>\n'
                 '  <path d="M12 7 V12 L15 14" %s/>' % (STROKE, STROKE))
glyph("downloads", '  <path d="M12 4 V15" %s/>\n'
                   '  <path d="M7 11 L12 16 L17 11" %s/>\n'
                   '  <path d="M5 20 H19" %s/>' % (STROKE, STROKE, STROKE))
glyph("menu", '  <circle cx="12" cy="6" r="1.6" fill="%s"/>'
              '<circle cx="12" cy="12" r="1.6" fill="%s"/>'
              '<circle cx="12" cy="18" r="1.6" fill="%s"/>' % (GREY, GREY, GREY))
glyph("private", '  <path d="M12 3 L20 6 V12 C20 17 16 20 12 21 '
                 'C8 20 4 17 4 12 V6 Z" %s/>' % STROKE)
glyph("reader", '  <path d="M4 6 H20" %s/>\n  <path d="M4 11 H20" %s/>\n'
                '  <path d="M4 16 H14" %s/>' % (STROKE, STROKE, STROKE))
glyph("find", '  <circle cx="10.5" cy="10.5" r="6" %s/>\n'
              '  <path d="M15 15 L20 20" %s/>' % (STROKE, STROKE))
glyph("sidebar", '  <rect x="3" y="4" width="18" height="16" rx="2" %s/>\n'
                 '  <path d="M9 4 V20" %s/>' % (STROKE, STROKE))
glyph("secure", '  <rect x="5" y="10" width="14" height="10" rx="2" %s/>\n'
                '  <path d="M8 10 V7 a4 4 0 0 1 8 0 V10" %s/>' % (STROKE, STROKE))
glyph("insecure", '  <path d="M12 3 L21 20 H3 Z" %s/>\n'
                  '  <path d="M12 10 V14" %s/>' % (STROKE, STROKE))
glyph("settings", '  <circle cx="12" cy="12" r="3.2" %s/>\n'
                  '  <path d="M12 3 V6 M12 18 V21 M3 12 H6 M18 12 H21" %s/>' % (STROKE, STROKE))

# ---------------------------------------------------------------------------
# New-tab page: lightweight, no analytics, no forced content.
# ---------------------------------------------------------------------------
write("resources/html/newtab.html", """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>New Tab</title>
<style>
  :root { color-scheme: light dark; }
  * { box-sizing: border-box; }
  body {
    margin: 0; min-height: 100vh; display: flex; flex-direction: column;
    align-items: center; justify-content: center; gap: 28px;
    font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
    background: light-dark(#fafafa, #1e1e1e);
    color: light-dark(#202124, #f5f5f5);
  }
  .brand { display: flex; align-items: center; gap: 12px; font-size: 22px; font-weight: 600; }
  .dot { width: 26px; height: 26px; border-radius: 8px;
         background: linear-gradient(135deg, #D88A24, #3D78B8); }
  form { width: min(560px, 86vw); display: flex; }
  input[type=text] {
    flex: 1; padding: 14px 16px; font-size: 16px; border-radius: 12px;
    border: 1px solid light-dark(#d8d8d8, #3a3a3a);
    background: light-dark(#ffffff, #252526); color: inherit;
  }
  input[type=text]:focus { outline: none; border-color: #3D78B8; }
  .links { display: flex; flex-wrap: wrap; gap: 12px; justify-content: center;
           width: min(640px, 90vw); }
  .links a {
    text-decoration: none; color: inherit; padding: 12px 16px; border-radius: 12px;
    background: light-dark(#ffffff, #252526);
    border: 1px solid light-dark(#e2e2e2, #333);
    min-width: 120px; text-align: center; font-size: 14px;
  }
  .links a:hover { border-color: #D88A24; }
  .note { font-size: 12px; opacity: 0.6; max-width: 520px; text-align: center; }
</style>
</head>
<body>
  <div class="brand"><span class="dot"></span> Polecat Browser</div>
  <form onsubmit="go(event)">
    <input id="q" type="text" autofocus
           placeholder="Search or enter a web address" aria-label="Search or enter a web address">
  </form>
  <div class="links">
    <a href="https://en.wikipedia.org">Wikipedia</a>
    <a href="https://www.khanacademy.org">Khan Academy</a>
    <a href="https://www.gutenberg.org">Gutenberg</a>
    <a href="https://arxiv.org">arXiv</a>
    <a href="https://en.wiktionary.org">Wiktionary</a>
  </div>
  <p class="note">This page makes no network requests of its own and stores
     nothing on your device.</p>
<script>
  function go(e) {
    e.preventDefault();
    var v = document.getElementById('q').value.trim();
    if (!v) return;
    var looksLikeUrl = /^(https?:\\/\\/|about:|file:)/i.test(v) ||
                       (/^[^\\s]+\\.[^\\s]{2,}/.test(v) && v.indexOf(' ') === -1);
    if (looksLikeUrl) {
      location.href = /^[a-z]+:/i.test(v) ? v : ('https://' + v);
    } else {
      location.href = 'https://duckduckgo.com/?q=' + encodeURIComponent(v);
    }
  }
</script>
</body>
</html>
""")

# ---------------------------------------------------------------------------
# Friendly error page (used as a reference; WebEngine's own error page is also
# enabled). Kept simple and jargon-free.
# ---------------------------------------------------------------------------
write("resources/html/error.html", """<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><title>Page could not load</title>
<style>
 body{font-family:system-ui,sans-serif;background:light-dark(#fafafa,#1e1e1e);
      color:light-dark(#202124,#f5f5f5);display:flex;align-items:center;
      justify-content:center;height:100vh;margin:0}
 .card{max-width:520px;text-align:center;padding:32px}
 h1{font-size:20px} p{opacity:.75;line-height:1.5}
</style></head>
<body><div class="card">
 <h1>Polecat Browser could not load this page</h1>
 <p>Check your internet connection or try again. If the address looks wrong,
    check it for typing mistakes.</p>
</div></body></html>
""")

# ---------------------------------------------------------------------------
# Private-window explanation shown in the new-tab page of private windows.
# ---------------------------------------------------------------------------
write("resources/html/privatenote.html", """<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><title>Private browsing</title>
<style>
 body{font-family:system-ui,sans-serif;background:#1e1e1e;color:#f5f5f5;
      display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0}
 .card{max-width:560px;padding:32px;line-height:1.6}
 h1{font-size:20px;color:#D88A24}
 ul{padding-left:20px} li{margin:6px 0}
 .muted{opacity:.65;font-size:13px}
</style></head>
<body><div class="card">
 <h1>You are browsing privately</h1>
 <ul>
   <li>History, cookies, cache and form data from this window are not saved on this device.</li>
   <li>Downloads you start here are not added to your download list.</li>
   <li>Anything you bookmark is still saved, because you asked for it.</li>
 </ul>
 <p class="muted">Private browsing does not make you anonymous online. Your
 network, your employer, and the websites you visit may still be able to see
 your activity.</p>
</div></body></html>
""")

# ---------------------------------------------------------------------------
# Bundled filter list. Small, hand-curated sample of well-known advertising and
# tracking domains. Documented as a sample, not a complete blocklist.
# ---------------------------------------------------------------------------
write("resources/filters/easylist-sample.txt", """! Polecat Browser bundled filter list (sample)
! Source: hand-curated list of well-known advertising and tracking hosts.
! Updated: 2026-01-01
! This is a small sample, not a complete blocklist. The browser does not claim
! perfect blocking. Users can add their own rules at
!   ~/.config/polecat-browser/filters/user.txt
!
! Syntax supported:
!   ||domain^        block domain and its subdomains
!   substring        block any URL containing the substring
!   @@||domain^      exception (never block this domain)
!
! --- advertising networks ---
||doubleclick.net^
||googlesyndication.com^
||googleadservices.com^
||adservice.google.com^
||adsafeprotected.com^
||adnxs.com^
||criteo.com^
||criteo.net^
||taboola.com^
||outbrain.com^
||pubmatic.com^
||rubiconproject.com^
||openx.net^
||casalemedia.com^
||smartadserver.com^
||adcolony.com^
||mopub.com^
!
! --- analytics and tracking ---
||google-analytics.com^
||googletagmanager.com^
||scorecardresearch.com^
||quantserve.com^
||hotjar.com^
||mixpanel.com^
||segment.io^
||amplitude.com^
||branch.io^
||newrelic.com^
||bugsnag.com^
||sentry.io^
!
! --- social trackers ---
||connect.facebook.net^
||pixel.facebook.com^
||ads.linkedin.com^
||analytics.twitter.com^
||static.ads-twitter.com^
!
! --- exceptions: never break these ---
@@||google.com^
@@||youtube.com^
@@||wikipedia.org^
""")

# ---------------------------------------------------------------------------
# Translation sources (structure only; .qm files are built by lrelease).
# ---------------------------------------------------------------------------
write("resources/translations/polecat_en.ts", """<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_IN">
<context>
    <name>Polecat</name>
    <message>
        <source>Polecat Browser</source>
        <translation>Polecat Browser</translation>
    </message>
</context>
</TS>
""")

write("resources/translations/polecat_hi.ts", """<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="hi_IN">
<context>
    <name>Polecat</name>
    <message>
        <source>Polecat Browser</source>
        <translation>पोलकैट ब्राउज़र</translation>
    </message>
    <message>
        <source>New Tab</source>
        <translation>नया टैब</translation>
    </message>
    <message>
        <source>Private</source>
        <translation>निजी</translation>
    </message>
</context>
</TS>
""")

print("resources generated")
