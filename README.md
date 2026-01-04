# Spectacle Patched

A patched version of [KDE Spectacle](https://invent.kde.org/plasma/spectacle) with an additional clipboard option.

## Patch: Copy File URI to Clipboard

This fork adds a new option **"Copy file URI to clipboard"** in Settings > General > After taking a screenshot.

- **Copy file location to clipboard** - Copies raw path: `/home/user/Screenshots/file.png`
- **Copy file URI to clipboard** (new) - Copies as URI with `text/uri-list` MIME type: `file:///home/user/Screenshots/file.png`

The URI format is recognized by file managers and other applications as a proper file reference.

## Installation (Arch Linux)

```bash
cd /path/to/this/repo
makepkg -si
```

## Updating

When upstream Spectacle updates:
1. Update `pkgver` in `PKGBUILD`
2. Run `makepkg -si`
3. If patch fails to apply, update `copy-file-uri.patch`

## Reverting to Official Spectacle

```bash
sudo pacman -S spectacle
```

---

# <img src="logo.png" width="48"/> Spectacle - The KDE Screenshot Utility

Spectacle is a screenshot taking utility for the KDE desktop. Spectacle
can also be used in non-KDE X11 desktop environments.

![Screenshot of Spectacle](https://cdn.kde.org/screenshots/spectacle/spectacle.png)

## Get help
You can get help in :
* Forum: https://discuss.kde.org/tag/spectacle
* Matrix: https://matrix.to/#/#kde:kde.org
* IRC: irc://irc.libera.chat/kde
## Contributing

Spectacle is developed under the KDE umbrella and uses KDE infrastructure
for development.

Please see the file [`CONTRIBUTING`](./CONTRIBUTING.md) for details on coding style and how
to contribute patches. Please note that pull requests on GitHub aren't
supported. The recommended way of contributing patches is via KDE's
instance of GitLab at https://invent.kde.org/plasma/spectacle.

## Release Schedule

Spectacle is released by KDE's release service and has three
major releases every year. They are numbered YY.MM, where YY is the two-
digit year and MM is the two-digit month. Major releases are made in April,
August and December every year. The Spectacle version follows the KDE
release service version.

## Reporting Bugs

Please report bugs at KDE's Bugzilla, available at https://bugs.kde.org/.

For discussions, the `#kde-devel` IRC channel and the kde-devel mailing list
are good places to post.
