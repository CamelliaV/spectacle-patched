include_directories(${PROJECT_SOURCE_DIR}/src)

SET(FILENAME_TEST_SRCS
    FilenameTest.cpp
    ../src/ShortcutActions.cpp
    ../src/ExportManager.cpp
    ../src/Platforms/ImagePlatform.cpp
    ../src/Platforms/VideoPlatform.cpp
)

ecm_qt_declare_logging_category(FILENAME_TEST_SRCS
    HEADER spectacle_debug.h
    IDENTIFIER SPECTACLE_LOG
    CATEGORY_NAME spectacle
    DESCRIPTION "spectacle (general)"
    EXPORT SPECTACLE
)

kconfig_add_kcfg_files(FILENAME_TEST_SRCS GENERATE_MOC ${PROJECT_SOURCE_DIR}/src/Gui/SettingsDialog/settings.kcfgc)

ecm_add_test(
    ${FILENAME_TEST_SRCS}
    TEST_NAME "filename_test"
    LINK_LIBRARIES  Qt::Test
        Qt::PrintSupport Qt::Qml KF6::I18n KF6::ConfigCore KF6::GlobalAccel KF6::KIOCore KF6::WindowSystem KF6::XmlGui KF6::GuiAddons KF6::PrisonScanner
)

ecm_add_test(
    OcrTest.cpp
    ../src/OcrManager.cpp
    ../src/MultilingualOcr.cpp
    ../src/OcrUtils.cpp
    ../src/TesseractRuntimeLoader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/settings.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/spectacle_debug.cpp
    TEST_NAME ocr_test
    LINK_LIBRARIES Qt::Test Qt::Widgets Qt::Qml KF6::I18n KF6::ConfigGui
)
target_include_directories(ocr_test PRIVATE ${PROJECT_BINARY_DIR}/src)
target_compile_options(ocr_test PRIVATE -fexceptions)

ecm_add_test(
    RegionDetectorTest.cpp
    ../src/Gui/RegionDetector.cpp
    ../src/OcrUtils.cpp
    TEST_NAME region_detector_test
    LINK_LIBRARIES Qt::Test Qt::Widgets ${OpenCV_LIBRARIES}
)
target_include_directories(region_detector_test PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_options(region_detector_test PRIVATE -fexceptions)

ecm_add_test(
    GuiWorkflowTest.cpp
    ../src/Gui/PinnedImageWindow.cpp
    ../src/Gui/OcrResultDialog.cpp
    TEST_NAME gui_workflow_test
    LINK_LIBRARIES Qt::Test Qt::Widgets Qt::DBus KF6::I18n KF6::WindowSystem LayerShellQt::Interface
)
set_tests_properties(ocr_test region_detector_test gui_workflow_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

ecm_add_test(
    MultilingualOcrTest.cpp
    ../src/MultilingualOcr.cpp
    ../src/OcrManager.cpp
    ../src/OcrUtils.cpp
    ../src/TesseractRuntimeLoader.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/settings.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/spectacle_debug.cpp
    TEST_NAME multilingual_ocr_test
    LINK_LIBRARIES Qt::Test Qt::Widgets Qt::Qml KF6::I18n KF6::ConfigGui
)
target_include_directories(multilingual_ocr_test PRIVATE ${PROJECT_BINARY_DIR}/src)
target_compile_options(multilingual_ocr_test PRIVATE -fexceptions)
target_compile_definitions(multilingual_ocr_test PRIVATE SPECTACLE_TEST_OCR_HELPER="${PROJECT_SOURCE_DIR}/src/Ocr/recognize.py")
set_tests_properties(multilingual_ocr_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME ocr_layout_test
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/OcrLayoutTest.py ${PROJECT_SOURCE_DIR}/src/Ocr/recognize.py)
