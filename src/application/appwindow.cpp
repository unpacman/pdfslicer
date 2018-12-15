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

#include "appwindow.hpp"
#include "aboutdialog.hpp"
#include "openfiledialog.hpp"
#include "savefiledialog.hpp"
#include <pdfsaver.hpp>
#include <glibmm/main.h>
#include <glibmm/i18n.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/messagedialog.h>
#include <config.hpp>
#include <logger.hpp>

namespace Slicer {

const std::set<int> AppWindow::zoomLevels = {200, 300, 400};

AppWindow::AppWindow(BackgroundThread& backgroundThread, SettingsManager& settingsManager)
    : m_backgroundThread{backgroundThread}
    , m_settingsManager{settingsManager}
    , m_windowState{}
    , m_view{m_backgroundThread}
    , m_zoomLevel{zoomLevels, *this}
{
    set_size_request(500, 500);

    loadWindowState();
    loadWidgets();
    addActions();
    setupWidgets();
    setupSignalHandlers();
    loadCustomCSS();

    show_all_children();
}

AppWindow::~AppWindow()
{
    saveWindowState();
}

void AppWindow::setDocument(std::unique_ptr<Document> document)
{
    m_view.setDocument(*document, m_zoomLevel.currentLevel());
    m_document = std::move(document);

    m_stack.set_visible_child("editor");

    m_headerBar.set_subtitle(m_document->basename());

    m_saveAction->set_enabled();
    m_zoomLevel.enable();

    m_document->commandExecuted().connect(sigc::mem_fun(*this, &AppWindow::onCommandExecuted));
}

bool AppWindow::on_delete_event(GdkEventAny*)
{
    // TODO: Right now, when we are saving a document and the user
    // tries to close the window, the app does nothing. It doesn't
    // react at all.
    // Maybe we should tell the user visually that his request to
    // close the window was purposefully ignored.
    return m_isSavingDocument;
}

void AppWindow::loadWindowState()
{
    m_windowState = m_settingsManager.loadWindowState();

    set_default_size(m_windowState.width, m_windowState.height);

    if (m_windowState.isMaximized)
        maximize();
}

void AppWindow::saveWindowState()
{
    m_settingsManager.saveWindowState(m_windowState);
}

void AppWindow::loadWidgets()
{
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_resource("/pdfslicer/ui/shortcuts.ui");
    Gtk::ShortcutsWindow* shortcutsWindow;
    builder->get_widget("shortcuts-pdfslicer", shortcutsWindow);

    shortcutsWindow->set_transient_for(*this);

    m_shortcutsWindow.reset(shortcutsWindow);
}

void AppWindow::addActions()
{
    m_openAction = add_action("open-document", sigc::mem_fun(*this, &AppWindow::onOpenAction));
    m_saveAction = add_action("save-document", sigc::mem_fun(*this, &AppWindow::onSaveAction));
    m_undoAction = add_action("undo", sigc::mem_fun(*this, &AppWindow::onUndoAction));
    m_redoAction = add_action("redo", sigc::mem_fun(*this, &AppWindow::onRedoAction));
    m_removeSelectedAction = add_action("remove-selected", sigc::mem_fun(*this, &AppWindow::onRemoveSelectedPages));
    m_removeUnselectedAction = add_action("remove-unselected", sigc::mem_fun(*this, &AppWindow::onRemoveUnselectedPages));
    m_removePreviousAction = add_action("remove-previous", sigc::mem_fun(*this, &AppWindow::onRemovePreviousPages));
    m_removeNextAction = add_action("remove-next", sigc::mem_fun(*this, &AppWindow::onRemoveNextPages));
    m_rotateRightAction = add_action("rotate-right", sigc::mem_fun(*this, &AppWindow::onRotatePagesRight));
    m_rotateLeftAction = add_action("rotate-left", sigc::mem_fun(*this, &AppWindow::onRotatePagesLeft));
    m_cancelSelectionAction = add_action("cancel-selection", sigc::mem_fun(*this, &AppWindow::onCancelSelection));
    m_shortcutsAction = add_action("shortcuts", sigc::mem_fun(*this, &AppWindow::onShortcutsAction));
    m_aboutAction = add_action("about", sigc::mem_fun(*this, &AppWindow::onAboutAction));

    m_saveAction->set_enabled(false);
    m_undoAction->set_enabled(false);
    m_redoAction->set_enabled(false);
    m_removeSelectedAction->set_enabled(false);
    m_removeUnselectedAction->set_enabled(false);
    m_removePreviousAction->set_enabled(false);
    m_removeNextAction->set_enabled(false);
    m_rotateRightAction->set_enabled(false);
    m_rotateLeftAction->set_enabled(false);
    m_cancelSelectionAction->set_enabled(false);
}

void AppWindow::setupWidgets()
{
    set_titlebar(m_headerBar);

    m_scroller.add(m_view);
    auto editorBox = Gtk::manage(new Gtk::Box{Gtk::ORIENTATION_VERTICAL}); // NOLINT
    editorBox->pack_start(m_scroller);
    editorBox->pack_start(m_actionBar, Gtk::PACK_SHRINK);

    m_stack.add(m_welcomeScreen, "welcome");
    m_stack.add(*editorBox, "editor");

    m_overlay.add(m_stack);
    m_overlay.add_overlay(m_savingRevealer);

    add(m_overlay); // NOLINT
    show_all_children();
}

void AppWindow::setupSignalHandlers()
{
    m_view.selectedPagesChanged.connect([this]() {
        onSelectedPagesChanged();
    });

    m_zoomLevel.changed.connect([this](int targetSize) {
        m_view.changePageSize(targetSize);
    });

    m_savedDispatcher.connect([this]() {
        m_savingRevealer.saved();
        enableEditingActions();
    });

    m_savingFailedDispatcher.connect([this]() {
        m_savingRevealer.set_reveal_child(false);
        enableEditingActions();

        Gtk::MessageDialog errorDialog{_("The current document could not be saved"),
                                       false,
                                       Gtk::MESSAGE_ERROR,
                                       Gtk::BUTTONS_CLOSE,
                                       true};
        errorDialog.set_transient_for(*this);

        errorDialog.run();
    });

    signal_size_allocate().connect(sigc::mem_fun(*this, &AppWindow::onSizeAllocate));
    signal_window_state_event().connect(sigc::mem_fun(*this, &AppWindow::onWindowStateEvent));
}

void AppWindow::loadCustomCSS()
{
    auto screen = Gdk::Screen::get_default();
    auto provider = Gtk::CssProvider::create();
    provider->load_from_data(R"(
        overlay > revealer > box {
            border-radius: 0px 0px 11px 11px;
        }

        .welcome-label {
            font-size: 19px;
            font-weight: bold;
        }

        .pepino {
            padding: 3px;
        }
    )");
    Gtk::StyleContext::add_provider_for_screen(screen,
                                               provider,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void AppWindow::disableEditingActions()
{
    m_openAction->set_enabled(false);
    m_saveAction->set_enabled(false);
    m_undoAction->set_enabled(false);
    m_redoAction->set_enabled(false);
    m_removeSelectedAction->set_enabled(false);
    m_removeUnselectedAction->set_enabled(false);
    m_removePreviousAction->set_enabled(false);
    m_removeNextAction->set_enabled(false);
    m_rotateRightAction->set_enabled(false);
    m_rotateLeftAction->set_enabled(false);
    m_cancelSelectionAction->set_enabled(false);
}

void AppWindow::enableEditingActions()
{
    m_openAction->set_enabled();
    m_saveAction->set_enabled();
    onSelectedPagesChanged();
    onCommandExecuted();
}

void AppWindow::onAboutAction()
{
    (new Slicer::AboutDialog{*this})->present();
}

void AppWindow::onShortcutsAction()
{
    m_shortcutsWindow->present();
    m_shortcutsWindow->show_all_children();
}

void AppWindow::onSaveAction()
{
    Slicer::SaveFileDialog dialog{*this};

    const int result = dialog.run();

    if (result == GTK_RESPONSE_ACCEPT)
        trySaveDocument(dialog.get_file());
}

void AppWindow::trySaveDocument(Glib::RefPtr<Gio::File> file)
{
    m_savingRevealer.saving();
    disableEditingActions();
    m_isSavingDocument = true;

    std::thread thread{[this, file]() {
        try {
            PdfSaver{*m_document}.save(file);
            m_savedDispatcher.emit();
        }
        catch (...) {
            Logger::logError("Saving the document failed");
            Logger::logError("The destination file was: " + file->get_path());
            m_savingFailedDispatcher.emit();
        }

        m_isSavingDocument = false;
    }};

    thread.detach();
}

void AppWindow::onOpenAction()
{
    Slicer::OpenFileDialog dialog{*this};

    const int result = dialog.run();

    if (result == GTK_RESPONSE_ACCEPT)
        tryOpenDocument(dialog.get_file());
}

void AppWindow::tryOpenDocument(Glib::RefPtr<Gio::File> file)
{
    try {
        m_undoAction->set_enabled(false);
        m_redoAction->set_enabled(false);
        auto document = std::make_unique<Document>(file);
        setDocument(std::move(document));
    }
    catch (...) {
        Logger::logError("The file couldn't be opened");
        Logger::logError("Filepath: " + file->get_path());

        Gtk::MessageDialog errorDialog{_("The selected file could not be opened"),
                                       false,
                                       Gtk::MESSAGE_ERROR,
                                       Gtk::BUTTONS_CLOSE,
                                       true};
        errorDialog.set_transient_for(*this);
        errorDialog.run();
    }
}

void AppWindow::onUndoAction()
{
    m_document->undoCommand();
}

void AppWindow::onRedoAction()
{

    m_document->redoCommand();
}

void AppWindow::onRemoveSelectedPages()
{
    m_document->removePages(m_view.getSelectedChildrenIndexes());
}

void AppWindow::onRemoveUnselectedPages()
{
    m_document->removePages(m_view.getUnselectedChildrenIndexes());
}

void AppWindow::onRemovePreviousPages()
{
    m_document->removePageRange(0, m_view.getSelectedChildIndex() - 1);
}

void AppWindow::onRemoveNextPages()
{
    m_document->removePageRange(m_view.getSelectedChildIndex() + 1,
                                static_cast<int>(m_document->pages()->get_n_items()) - 1);
}

void AppWindow::onRotatePagesRight()
{
    m_document->rotatePagesRight(m_view.getSelectedChildrenIndexes());
}

void AppWindow::onRotatePagesLeft()
{
    m_document->rotatePagesLeft(m_view.getSelectedChildrenIndexes());
}

void AppWindow::onCancelSelection()
{
    m_view.clearSelection();
}

void AppWindow::onSelectedPagesChanged()
{
    const unsigned long numSelected = m_view.getSelectedChildrenIndexes().size();

    if (numSelected == 0) {
        m_removeSelectedAction->set_enabled(false);
        m_removeUnselectedAction->set_enabled(false);
        m_removePreviousAction->set_enabled(false);
        m_removeNextAction->set_enabled(false);
        m_rotateRightAction->set_enabled(false);
        m_rotateLeftAction->set_enabled(false);
        m_cancelSelectionAction->set_enabled(false);
    }
    else {
        m_removeSelectedAction->set_enabled();
        m_rotateRightAction->set_enabled();
        m_rotateLeftAction->set_enabled();
        m_removeUnselectedAction->set_enabled();
        m_cancelSelectionAction->set_enabled();
    }

    if (numSelected == 1) {
        const int index = m_view.getSelectedChildIndex();
        if (index == 0)
            m_removePreviousAction->set_enabled(false);
        else
            m_removePreviousAction->set_enabled();

        const int numPages = static_cast<int>(m_view.get_children().size());
        if (index == numPages - 1)
            m_removeNextAction->set_enabled(false);
        else
            m_removeNextAction->set_enabled();
    }

    if (numSelected > 1) {
        m_removePreviousAction->set_enabled(false);
        m_removeNextAction->set_enabled(false);
    }
}

void AppWindow::onCommandExecuted()
{
    if (m_document->canUndo())
        m_undoAction->set_enabled();
    else
        m_undoAction->set_enabled(false);

    if (m_document->canRedo())
        m_redoAction->set_enabled();
    else
        m_redoAction->set_enabled(false);
}

void AppWindow::onSizeAllocate(Gtk::Allocation&)
{
    if (!is_maximized())
        get_size(m_windowState.width, m_windowState.height);
}

bool AppWindow::onWindowStateEvent(GdkEventWindowState* state)
{
    m_windowState.isMaximized = (state->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;

    return false;
}
}