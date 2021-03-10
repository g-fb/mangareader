
#### Donate: [GitHub Sponsors](https://github.com/sponsors/g-fb) | [Liberapay](https://liberapay.com/gfb/) | [PayPal](https://paypal.me/georgefloreabanus) | [Patreon](https://www.patreon.com/georgefb)

# Manga Reader

A manga reader for local files. Works with folders and archives.

# Installation

Easiest way to install is through [flatpak](https://flathub.org/apps/details/com.georgefb.mangareader), flatpaks should work on all distros.

[Flatpak setup guide](https://flatpak.org/setup/)
```
flatpak install flathub com.georgefb.mangareader
flatpak run com.georgefb.mangareader
```

If you don't like flatpak you can build from source or ask the your distro to provide a package.

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

```
git clone https://gitlab.com/g-fb/manga-reader
cd manga-reader && mkdir build && cd build
cmake ..
cmake --build .
./mangareader

```

# Screenshots

![Manga Reader main window](data/images/manga-reader--dark.png)
![Manga Reader settings dialog](data/images/manga-reader--main-window-and-settings.png)
![Manga Reader with images](data/images/manga-reader--light.png)
