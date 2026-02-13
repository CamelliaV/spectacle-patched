# Maintainer: Cindy <cameliascript@gmail.com>
# Contributor: Based on official Arch Linux spectacle PKGBUILD

pkgname=spectacle-patched
_pkgname=spectacle
pkgver=6.5.5
pkgrel=2
epoch=1
pkgdesc='KDE screenshot capture utility (with copy file URI patch)'
arch=(x86_64)
url='https://kde.org/plasma-desktop/'
license=(GPL-2.0-or-later)
depends=(gcc-libs
         glibc
         kconfig
         kconfigwidgets
         kcoreaddons
         kcrash
         kdbusaddons
         kglobalaccel
         kguiaddons
         ki18n
         kio
         kirigami
         kjobwidgets
         knotifications
         kpipewire
         kservice
         kstatusnotifieritem
         kwidgetsaddons
         kwindowsystem
         kxmlgui
         layer-shell-qt
         libxcb
         opencv
         prison
         purpose
         qt6-base
         qt6-declarative
         qt6-imageformats
         qt6-multimedia
         wayland
         xcb-util
         xcb-util-cursor
         xcb-util-image)
makedepends=(extra-cmake-modules
             kdoctools
             kquickimageeditor
             plasma-wayland-protocols
             wayland-protocols)
optdepends=('tesseract: OCR text recognition'
            'tessdata: OCR language data')
provides=(spectacle)
conflicts=(spectacle)
groups=(plasma)
source=(https://download.kde.org/stable/plasma/$pkgver/$_pkgname-$pkgver.tar.xz{,.sig}
        copy-file-uri.patch
        game-mode-shortcut-suppression.patch)
sha256sums=('SKIP'
            'SKIP'
            'SKIP'
            'SKIP')
validpgpkeys=(E0A3EB202F8E57528E13E72FD7574483BB57B18D  # Jonathan Esk-Riddell <jr@jriddell.org>
              0AAC775BB6437A8D9AF7A3ACFE0784117FBCE11D  # Bhushan Shah <bshah@kde.org>
              D07BD8662C56CB291B316EB2F5675605C74E02CF  # David Edmundson <davidedmundson@kde.org>
              1FA881591C26B276D7A5518EEAAF29B42A678C20  # Marco Martin <notmart@gmail.com>
              B3CB366552540BE06EE9AD9711968C44928CAEFC) # KDE release signing key (Plasma 6.5.5)

prepare() {
  cd $_pkgname-$pkgver
  patch -Np1 -i ../copy-file-uri.patch
  patch -Np1 -i ../game-mode-shortcut-suppression.patch
}

build() {
  cmake -B build -S $_pkgname-$pkgver \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
