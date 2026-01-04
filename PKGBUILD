# Maintainer: Cindy (local patched build)
# Based on official Arch Linux PKGBUILD

pkgname=spectacle-patched
_pkgname=spectacle
pkgver=6.5.4
pkgrel=1
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
provides=(spectacle)
conflicts=(spectacle)
groups=(plasma)
source=(https://download.kde.org/stable/plasma/$pkgver/$_pkgname-$pkgver.tar.xz
        copy-file-uri.patch)
sha256sums=('SKIP'
            'SKIP')

prepare() {
  cd $_pkgname-$pkgver
  patch -Np1 -i ../copy-file-uri.patch
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
