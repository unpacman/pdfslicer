#include "settingsmanager.hpp"
#include <giomm/file.h>
#include <glibmm/miscutils.h>
#include <config.hpp>

namespace Slicer {

namespace window_state {
    static const std::string groupName = "window-state";

    static const struct {
        std::string width = "width";
        std::string height = "height";
        std::string isMaximized = "is-maximized";
    } keys;

    static const WindowState defaultWindowState = {800, 600, false};
}

SettingsManager::SettingsManager()
{
    loadConfigFile();
}

SettingsManager::~SettingsManager()
{
    saveConfigFile();
}

WindowState SettingsManager::loadWindowState()
{
    WindowState result;

    try {
        result.width = m_keyFile.get_integer(window_state::groupName, window_state::keys.width);
        result.height = m_keyFile.get_integer(window_state::groupName, window_state::keys.height);
        result.isMaximized = m_keyFile.get_boolean(window_state::groupName, window_state::keys.isMaximized);
    }
    catch (...) {
        result = window_state::defaultWindowState;
    }

    return result;
}

void SettingsManager::saveWindowState(const WindowState& windowState)
{
    m_keyFile.set_integer(window_state::groupName, window_state::keys.width, windowState.width);
    m_keyFile.set_integer(window_state::groupName, window_state::keys.height, windowState.height);
    m_keyFile.set_boolean(window_state::groupName, window_state::keys.isMaximized, windowState.isMaximized);
}

void SettingsManager::loadConfigFile()
{
    try {
        m_keyFile.load_from_file(getSettingsFilePath());
    }
    catch (...) {
    }
}

void SettingsManager::saveConfigFile()
{
    Glib::RefPtr<Gio::File> settingsDirectory = Gio::File::create_for_path(getSettingsParentPath());

    try {
        if (!settingsDirectory->query_exists())
            settingsDirectory->make_directory_with_parents();

        m_keyFile.save_to_file(getSettingsFilePath());
    }
    catch (...) {
    }
}

std::string SettingsManager::getSettingsParentPath() const
{
    return Glib::build_filename(std::vector<std::string>{Glib::get_user_cache_dir(),
                                                         config::APPLICATION_ID});
}

std::string SettingsManager::getSettingsFilePath() const
{
    return Glib::build_filename(std::vector<std::string>{getSettingsParentPath(),
                                                         "settings.ini"});
}

} // namespace Slicer