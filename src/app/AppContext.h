#pragma once

namespace Polecat {

class SettingsManager;
class ProfileManager;
class BookmarkManager;
class HistoryManager;
class DownloadsManager;
class PrivacyManager;
class PermissionManager;
class ContentBlocker;
class ThemeManager;
class DatabaseManager;

// AppContext is a plain bundle of the application-wide managers. It is passed
// to windows and dialogs so they can reach shared services without each window
// re-constructing them.
struct AppContext {
    SettingsManager *settings = nullptr;
    ProfileManager *profiles = nullptr;
    BookmarkManager *bookmarks = nullptr;
    HistoryManager *history = nullptr;
    DownloadsManager *downloads = nullptr;
    PrivacyManager *privacy = nullptr;
    PermissionManager *permissions = nullptr;
    ContentBlocker *blocker = nullptr;
    ThemeManager *theme = nullptr;
    DatabaseManager *database = nullptr;

    bool isValid() const
    {
        return settings && profiles && bookmarks && history && downloads
               && privacy && permissions && blocker && theme && database;
    }
};

} // namespace Polecat
