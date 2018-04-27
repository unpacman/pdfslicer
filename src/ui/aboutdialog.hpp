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

#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <gtkmm/aboutdialog.h>

namespace Slicer {

class AboutDialog : public Gtk::AboutDialog {
public:
    AboutDialog(Gtk::Window& parent);
    virtual ~AboutDialog() = default;
};

} // namespace Slicer

#endif // ABOUTDIALOG_HPP
