# Unit tests for Polecat Browser.
#
# Each test links only the sources it needs, so a test failure points at a
# small, well-defined unit. Tests that touch the filesystem enable Qt's
# "test mode" so they never read or write the user's real configuration.

find_package(Qt6 6.4 REQUIRED COMPONENTS Test Sql)

set(POLECAT_SRC ${CMAKE_SOURCE_DIR}/src)

# Common, dependency-light support sources.
set(POLECAT_TEST_SUPPORT
    ${POLECAT_SRC}/utils/Paths.cpp
    ${POLECAT_SRC}/utils/Logging.cpp
)

function(polecat_add_test name)
    set(sources ${ARGN})
    add_executable(${name} ${sources})
    target_include_directories(${name} PRIVATE ${POLECAT_SRC})
    target_compile_definitions(${name} PRIVATE
        POLECAT_VERSION="${PROJECT_VERSION}")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# --- URL utilities (no Qt Widgets, no database) ----------------------------
polecat_add_test(tst_urlutils
    tst_urlutils.cpp
    ${POLECAT_SRC}/utils/UrlUtils.cpp
    ${POLECAT_TEST_SUPPORT}
)
target_link_libraries(tst_urlutils PRIVATE Qt6::Core Qt6::Test)

# --- Settings persistence --------------------------------------------------
polecat_add_test(tst_settings
    tst_settings.cpp
    ${POLECAT_SRC}/settings/SettingsManager.cpp
    ${POLECAT_TEST_SUPPORT}
)
target_link_libraries(tst_settings PRIVATE Qt6::Core Qt6::Test)

# --- Database open + migrations --------------------------------------------
polecat_add_test(tst_database
    tst_database.cpp
    ${POLECAT_SRC}/database/DatabaseManager.cpp
    ${POLECAT_TEST_SUPPORT}
)
target_link_libraries(tst_database PRIVATE Qt6::Core Qt6::Sql Qt6::Test)

# --- Bookmarks -------------------------------------------------------------
polecat_add_test(tst_bookmarks
    tst_bookmarks.cpp
    ${POLECAT_SRC}/database/DatabaseManager.cpp
    ${POLECAT_SRC}/bookmarks/BookmarkManager.cpp
    ${POLECAT_SRC}/utils/UrlUtils.cpp
    ${POLECAT_TEST_SUPPORT}
)
target_link_libraries(tst_bookmarks PRIVATE Qt6::Core Qt6::Sql Qt6::Test)

# --- History ---------------------------------------------------------------
polecat_add_test(tst_history
    tst_history.cpp
    ${POLECAT_SRC}/database/DatabaseManager.cpp
    ${POLECAT_SRC}/history/HistoryManager.cpp
    ${POLECAT_SRC}/utils/UrlUtils.cpp
    ${POLECAT_TEST_SUPPORT}
)
target_link_libraries(tst_history PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
