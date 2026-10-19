cmake_minimum_required(VERSION 3.16)

project(polecat-browser
    VERSION 1.0.0
    DESCRIPTION "A modern, privacy-focused web browser for Debian Linux"
    HOMEPAGE_URL ""
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Language / build settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
include(FeatureSummary)

# ---------------------------------------------------------------------------
# Qt 6 — minimum 6.4 (Debian 12 / bookworm ships 6.4).
# We intentionally do NOT require anything newer so the browser keeps working
# on older GNOME/older Debian releases.
# ---------------------------------------------------------------------------
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    PrintSupport
    WebEngineWidgets
    WebEngineCore
)

# qt_standard_project_setup() exists from Qt 6.3; guard it anyway.
if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(POLECAT_SOURCES
    src/main.cpp

    src/app/Application.cpp

    src/browser/BrowserWindow.cpp
    src/browser/WebView.cpp
    src/browser/WebPage.cpp
    src/browser/NavigationToolBar.cpp
    src/browser/AddressBar.cpp

    src/tabs/TabManager.cpp
    src/tabs/BrowserTab.cpp
    src/tabs/TabBar.cpp

    src/downloads/DownloadsManager.cpp
    src/downloads/DownloadsPanel.cpp

    src/bookmarks/BookmarkManager.cpp
    src/bookmarks/BookmarksPanel.cpp

    src/history/HistoryManager.cpp
    src/history/HistoryPanel.cpp

    src/privacy/PrivacyManager.cpp
    src/privacy/ContentBlocker.cpp
    src/privacy/PermissionManager.cpp

    src/settings/SettingsManager.cpp
    src/settings/SettingsDialog.cpp

    src/profiles/ProfileManager.cpp

    src/ui/HeaderBar.cpp
    src/ui/Sidebar.cpp
    src/ui/FindBar.cpp
    src/ui/AboutDialog.cpp
    src/ui/TitleBar.cpp
    src/ui/ThemeManager.cpp

    src/database/DatabaseManager.cpp

    src/study/StudyPanel.cpp

    src/utils/Paths.cpp
    src/utils/UrlUtils.cpp
    src/utils/Logging.cpp
    src/utils/IconLoader.cpp
)

set(POLECAT_HEADERS
    src/app/Application.h
    src/app/AppContext.h

    src/browser/BrowserWindow.h
    src/browser/WebView.h
    src/browser/WebPage.h
    src/browser/NavigationToolBar.h
    src/browser/AddressBar.h

    src/tabs/TabManager.h
    src/tabs/BrowserTab.h
    src/tabs/TabBar.h

    src/downloads/DownloadsManager.h
    src/downloads/DownloadsPanel.h

    src/bookmarks/BookmarkManager.h
    src/bookmarks/BookmarksPanel.h

    src/history/HistoryManager.h
    src/history/HistoryPanel.h

    src/privacy/PrivacyManager.h
    src/privacy/ContentBlocker.h
    src/privacy/PermissionManager.h

    src/settings/SettingsManager.h
    src/settings/SettingsDialog.h

    src/profiles/ProfileManager.h

    src/ui/HeaderBar.h
    src/ui/Sidebar.h
    src/ui/FindBar.h
    src/ui/AboutDialog.h
    src/ui/TitleBar.h
    src/ui/ThemeManager.h

    src/database/DatabaseManager.h

    src/study/StudyPanel.h

    src/utils/Paths.h
    src/utils/UrlUtils.h
    src/utils/Logging.h
    src/utils/IconLoader.h
)

# ---------------------------------------------------------------------------
# Resources (icons, html, filters, translations)
# ---------------------------------------------------------------------------
qt_add_resources(POLECAT_RESOURCES resources/polecat.qrc)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
if(COMMAND qt_add_executable)
    qt_add_executable(polecat-browser ${POLECAT_SOURCES} ${POLECAT_HEADERS})
else()
    add_executable(polecat-browser ${POLECAT_SOURCES} ${POLECAT_HEADERS})
endif()

target_include_directories(polecat-browser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(polecat-browser PRIVATE
    POLECAT_VERSION="${PROJECT_VERSION}"
    POLECAT_APP_ID="org.opentiaos.PolecatBrowser"
    POLECAT_DEVELOPER="OpentiaOS Developer"
    POLECAT_DEVELOPER_EMAIL="opentiaos@gmail.com"
)

target_link_libraries(polecat-browser PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
)

# ---------------------------------------------------------------------------
# Install rules (used both by `make install` and by CPack DEB)
# ---------------------------------------------------------------------------
install(TARGETS polecat-browser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
    packaging/org.opentiaos.PolecatBrowser.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(FILES
    packaging/org.opentiaos.PolecatBrowser.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)

install(FILES
    resources/icons/org.opentiaos.PolecatBrowser.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)

# Man page
install(FILES docs/polecat-browser.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(POLECAT_BUILD_TESTS "Build unit tests" ON)
if(POLECAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# CPack — produce a Debian .deb (the ONLY supported package format)
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "polecat-browser")
set(CPACK_PACKAGE_VENDOR "OpentiaOS Developer")
set(CPACK_PACKAGE_CONTACT "OpentiaOS Developer <opentiaos@gmail.com>")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast, privacy-focused web browser for Debian Linux")
set(CPACK_PACKAGE_HOMEPAGE_URL "")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "polecat-browser")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "OpentiaOS Developer <opentiaos@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6 (>= 6.4), libqt6gui6 (>= 6.4), libqt6widgets6 (>= 6.4), \
libqt6network6 (>= 6.4), libqt6sql6 (>= 6.4), libqt6sql6-sqlite (>= 6.4), \
libqt6printsupport6 (>= 6.4), libqt6webenginecore6 (>= 6.4), \
libqt6webenginewidgets6 (>= 6.4), libqt6svg6 (>= 6.4)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-wayland")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_GENERATOR "DEB")
# Use the standard Debian package filename: polecat-browser_1.0.0_amd64.deb
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEB_COMPONENT_INSTALL OFF)

include(CPack)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
