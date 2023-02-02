<!--
SPDX-FileCopyrightText: 2020 George Florea Bănuș <georgefb899@gmail.com>

SPDX-License-Identifier: CC-BY-SA-4.0
-->
#### Donate: [GitHub Sponsors](https://github.com/sponsors/g-fb) | [Liberapay](https://liberapay.com/gfb/) | [PayPal](https://paypal.me/georgefloreabanus)

# Manga Reader

A manga reader for local files. Works with folders and archives.

# Status

The application is in maintenance mode, that means only bugfixes will happen.

From my point of view the application is feature complete, so no more features from me. If someone else wants to add a feature they are welcome to submit a PR, but before doing that open an issue to check if the feature is gonna be accepted.

You can still make feature requests and if it's easy to implement I might add it, just don't expect them to be implemented.

# Installation

Easiest way to install is through [flatpak](https://flathub.org/apps/details/com.georgefb.mangareader), flatpaks should work on all distros.

[Flatpak setup guide](https://flatpak.org/setup/)
```
flatpak install flathub com.georgefb.mangareader
flatpak run com.georgefb.mangareader
```

If you don't like flatpak you can build from source or ask your distro to provide a package.

# Dependencies

- Extra CMake Modules
- Qt5 Widgets
- KF5 XmlGui
- KF5 ConfigCore
- KF5 CoreAddons
- KF5 I18n
- libarchive

---------------------------------

- c++ compiler
- cmake
- git

# Build

```bash
git clone https://github.com/g-fb/mangareader
cd manga-reader
# append `-D CMAKE_INSTALL_PREFIX:PATH=/your/custom/path` to install to a custom location
cmake -B build -G Ninja
cmake --build build
cmake --install build
```

# Screenshots

![Manga Reader main window](data/images/manga-reader--dark.png)
![Manga Reader settings dialog](data/images/manga-reader--main-window-and-settings.png)
![Manga Reader with images](data/images/manga-reader--light.png)
