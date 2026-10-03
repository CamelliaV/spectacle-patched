# Maintainer: Cindy <cameliascript@gmail.com>
# Contributor: Based on official Arch Linux spectacle PKGBUILD

pkgname=spectacle-patched
_pkgname=spectacle
pkgver=6.7.5
pkgrel=6
epoch=1
pkgdesc='KDE screenshot capture utility with patched workflow features'
arch=(x86_64)
url='https://kde.org/plasma-desktop/'
license=(GPL-2.0-or-later Apache-2.0)
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
         python
         python-onnxruntime-cpu
         python-numpy
         python-pyclipper
         python-shapely
         python-omegaconf
         python-colorlog
         python-requests
         python-pillow
         python-yaml
         python-tqdm
         python-six
         purpose
         qt6-base
         qt6-declarative
         qt6-imageformats
         qt6-multimedia
         tesseract
         tesseract-data-eng
         tesseract-data-chi_sim
         tesseract-data-chi_tra
         tesseract-data-jpn
         wayland
         xcb-util
         xcb-util-cursor
         xcb-util-image)
makedepends=(extra-cmake-modules
             kdoctools
             kquickimageeditor
             plasma-wayland-protocols
             wayland-protocols)
optdepends=('tessdata: Additional OCR languages')
provides=(spectacle)
conflicts=(spectacle)
groups=(plasma)
source=(https://download.kde.org/stable/plasma/$pkgver/$_pkgname-$pkgver.tar.xz{,.sig}
        copy-file-uri.patch
        game-mode-shortcut-suppression.patch
        restore-last-selection-rect.patch
        empty-selection-current-screen.patch
        ocr-pin-smart-regions.patch
        rapidocr-3.9.2.whl::https://files.pythonhosted.org/packages/55/ed/0ee9b9281986974be9d2406ae0134c8d7c91d2fc613f16ffda9701eeda6f/rapidocr-3.9.2-py3-none-any.whl
        opencv-python-headless-4.12.0.88.whl::https://files.pythonhosted.org/packages/89/53/e19c21e0c4eb1275c3e2c97b081103b6dfb3938172264d283a519bf728b9/opencv_python_headless-4.12.0.88-cp37-abi3-manylinux2014_x86_64.manylinux_2_17_x86_64.whl
        ch_PP-OCRv5_det_mobile.onnx::https://www.modelscope.cn/models/RapidAI/RapidOCR/resolve/v3.9.2/onnx/PP-OCRv5/det/ch_PP-OCRv5_det_mobile.onnx
        ch_PP-OCRv5_rec_server.onnx::https://www.modelscope.cn/models/RapidAI/RapidOCR/resolve/v3.9.2/onnx/PP-OCRv5/rec/ch_PP-OCRv5_rec_server.onnx
        ch_PP-LCNet_x0_25_textline_ori_cls_mobile.onnx::https://www.modelscope.cn/models/RapidAI/RapidOCR/resolve/v3.9.2/onnx/PP-OCRv5/cls/ch_PP-LCNet_x0_25_textline_ori_cls_mobile.onnx
        en_PP-OCRv5_rec_mobile.onnx::https://www.modelscope.cn/models/RapidAI/RapidOCR/resolve/v3.9.2/onnx/PP-OCRv5/rec/en_PP-OCRv5_rec_mobile.onnx)
sha256sums=(
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            'SKIP'
            '04d6b8d151f823d930bd91910555f57bea897c0c44fa6794267b94cf9c1ef9a0'
            '236c8df54a90f4d02076e6f9c1cc763d794542e886c576a6fee46ec8ff75a7a9'
            '4d97c44a20d30a81aad087d6a396b08f786c4635742afc391f6621f5c6ae78ae'
            'e09385400eaaaef34ceff54aeb7c4f0f1fe014c27fa8b9905d4709b65746562a'
            '54379ae5174d026780215fc748a7f31910dee36818e63d49e17dc598ecc82df7'
            'c3461add59bb4323ecba96a492ab75e06dda42467c9e3d0c18db5d1d21924be8'
)
noextract=(rapidocr-3.9.2.whl opencv-python-headless-4.12.0.88.whl ch_PP-OCRv5_det_mobile.onnx ch_PP-OCRv5_rec_server.onnx ch_PP-LCNet_x0_25_textline_ori_cls_mobile.onnx en_PP-OCRv5_rec_mobile.onnx)

validpgpkeys=(E0A3EB202F8E57528E13E72FD7574483BB57B18D  # Jonathan Esk-Riddell <jr@jriddell.org>
              0AAC775BB6437A8D9AF7A3ACFE0784117FBCE11D  # Bhushan Shah <bshah@kde.org>
              D07BD8662C56CB291B316EB2F5675605C74E02CF  # David Edmundson <davidedmundson@kde.org>
              1FA881591C26B276D7A5518EEAAF29B42A678C20  # Marco Martin <notmart@gmail.com>
              B3CB366552540BE06EE9AD9711968C44928CAEFC) # KDE release signing key (Plasma 6.7.5)

prepare() {
  cd $_pkgname-$pkgver
  patch -Np1 -i ../copy-file-uri.patch
  patch -Np1 -i ../game-mode-shortcut-suppression.patch
  patch -Np1 -i ../restore-last-selection-rect.patch
  patch -Np1 -i ../empty-selection-current-screen.patch
  patch -Np1 -i ../ocr-pin-smart-regions.patch
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
  local ocrdir="$pkgdir/usr/share/spectacle/ocr"
  install -d "$ocrdir/vendor" "$ocrdir/models"
  bsdtar -xf "$srcdir/rapidocr-3.9.2.whl" -C "$ocrdir/vendor"
  bsdtar -xf "$srcdir/opencv-python-headless-4.12.0.88.whl" -C "$ocrdir/vendor"
  install -m644 "$srcdir/"*.onnx "$ocrdir/models/"
}
