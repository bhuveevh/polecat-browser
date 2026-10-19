# Getting a .deb from GitHub

This repository is ready to build Polecat Browser's `.deb` on GitHub, with no
local toolchain needed.

## Push it up

```bash
git init
git add .
git commit -m "Polecat Browser 1.0.0"
git branch -M main
git remote add origin https://github.com/<your-user>/<your-repo>.git
git push -u origin main
```

## Run the build

Open the **Actions** tab -> **Build Debian package** -> **Run workflow**.

When it finishes (~5-10 min), open the run and download the
**polecat-browser-deb** artifact. Inside it is:

    polecat-browser_1.0.0_amd64.deb

## Or build a tagged release

```bash
git tag v1.0.0
git push origin v1.0.0
```

The workflow then also attaches the `.deb` to a GitHub Release automatically.

## Install the result (on a Debian 12 machine)

```bash
sudo apt install ./polecat-browser_1.0.0_amd64.deb
polecat-browser
```

The workflow runs inside a `debian:bookworm` container, so the package is built
against Debian 12's Qt 6.4 and installs cleanly on bookworm.
