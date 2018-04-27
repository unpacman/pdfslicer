// PDF Slicer
// Copyright (C) 2017-2018 Julián Unrrein

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PREVIEWWINDOW_HPP
#define PREVIEWWINDOW_HPP

#include "../backend/page.hpp"
#include "zoomlevelwithactions.hpp"
#include <gtkmm/window.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/image.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/overlay.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>

namespace Slicer {

class PreviewWindow : public Gtk::Window {
public:
    PreviewWindow(Glib::RefPtr<Page> page);
    virtual ~PreviewWindow() = default;

private:
    Glib::RefPtr<Page> m_page;
    Glib::RefPtr<Gio::SimpleActionGroup> m_actionGroup;
    ZoomLevelWithActions m_zoomLevel;
    static const std::set<int> zoomLevels;

    Gtk::Image m_image;
    Gtk::ScrolledWindow m_scroller;
    Gtk::Overlay m_overlay;
    Gtk::Button m_buttonZoomIn;
    Gtk::Button m_buttonZoomOut;
    Gtk::Box m_boxZoom;

    void setupWidgets();
    void setupSignalHandlers();
    void loadCustomCSS();
};

} // namespace Slicer

#endif // BIGPREVIEWWINDOW_HPP
