#include <gtk/gtk.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{

    int &editor_window_count()
    {
        static int count = 0;
        return count;
    }

    struct Editor
    {
        GtkWidget *window;
        GtkWidget *view;
        GtkWidget *status;
        GtkWidget *document_status;
        GtkWidget *toolbar = nullptr;
        GtkWidget *status_bar = nullptr;
        GtkCssProvider *zoom_provider = nullptr;
        GtkWidget *notebook = nullptr;
        GtkWidget *page = nullptr;
        GtkWidget *tab_label = nullptr;
        Editor *workspace = nullptr;
        Editor *active_tab = nullptr;
        Editor *help_tab = nullptr;
        std::string filename;
        std::string line_ending = "LF";
        int zoom = 14;
        bool dirty = false;
        bool loading = false;
        bool ibus_active = false;
        bool help = false;

        ~Editor()
        {
            if (zoom_provider != nullptr)
                g_object_unref(zoom_provider);
        }
    };

    Editor *document(Editor *editor)
    {
        if (editor == nullptr)
            return nullptr;

        if (editor->workspace != nullptr)
            return editor;
        return editor->active_tab;
    }

    Editor *workspace(Editor *editor)
    {
        if (editor == nullptr)
            return nullptr;
        return editor->workspace != nullptr ? editor->workspace : editor;
    }

    GtkTextBuffer *text_buffer(Editor *editor)
    {
        Editor *tab = document(editor);
        return gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->view));
    }

    std::string buffer_text(Editor *editor)
    {
        GtkTextBuffer *buffer = text_buffer(editor);
        GtkTextIter start;
        GtkTextIter end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);

        gchar *value = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        std::string result = value != nullptr ? value : "";
        g_free(value);
        return result;
    }

    void update_window_title(Editor *editor)
    {
        Editor *tab = document(editor);
        Editor *app = workspace(editor);
        std::string name = "Untitled";
        if (!tab->filename.empty())
        {
            gchar *basename = g_path_get_basename(tab->filename.c_str());
            name = basename != nullptr ? basename : "Untitled";
            g_free(basename);
        }
        const std::string title =
            (tab->dirty ? "*" : "") + name + " - Nudi Kannada Editor";
        gtk_window_set_title(GTK_WINDOW(app->window), title.c_str());

        if (tab->tab_label != nullptr)
        {
            const std::string tab_title =
                (tab->dirty ? "*" : "") + name;
            gtk_label_set_text(GTK_LABEL(tab->tab_label), tab_title.c_str());
        }
    }

    void update_status(Editor *editor, const char *message = nullptr)
    {
        Editor *app = workspace(editor);
        if (message != nullptr)
        {
            gtk_label_set_text(GTK_LABEL(app->document_status), message);
            return;
        }

        gtk_label_set_text(
            GTK_LABEL(app->document_status),
            app->ibus_active ? "INS  · Nudi active" : "INS");
    }

    void update_position_status(Editor *editor)
    {
        Editor *app = workspace(editor);
        GtkTextBuffer *buffer = text_buffer(editor);
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_mark(
            buffer,
            &cursor,
            gtk_text_buffer_get_insert(buffer));

        const int line = gtk_text_iter_get_line(&cursor) + 1;
        const int column = gtk_text_iter_get_line_offset(&cursor) + 1;
        const int characters = gtk_text_buffer_get_char_count(buffer);

        std::string status =
            "Ln " + std::to_string(line) +
            ", Col " + std::to_string(column) +
            "  |  " + std::to_string(characters) + " chars";
        gtk_label_set_text(GTK_LABEL(app->status), status.c_str());

        std::string document_status = "Help";
        if (!document(editor)->help)
        {
            document_status =
                document(editor)->line_ending + " · UTF-8    INS";
        }
        gtk_label_set_text(
            GTK_LABEL(app->document_status),
            document_status.c_str());
    }

    void apply_zoom(Editor *editor)
    {
        Editor *tab = document(editor);
        GtkStyleContext *context =
            gtk_widget_get_style_context(tab->view);

        if (tab->zoom_provider != nullptr)
        {
            gtk_style_context_remove_provider(
                context,
                GTK_STYLE_PROVIDER(tab->zoom_provider));
            g_object_unref(tab->zoom_provider);
            tab->zoom_provider = nullptr;
        }

        tab->zoom_provider = gtk_css_provider_new();
        const std::string css =
            "#nudi-editor { font-family: Sans; font-size: " +
            std::to_string(tab->zoom) + "pt; }";
        gtk_css_provider_load_from_data(
            tab->zoom_provider,
            css.c_str(),
            -1,
            nullptr);
        gtk_style_context_add_provider(
            context,
            GTK_STYLE_PROVIDER(tab->zoom_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        update_position_status(tab);
    }

    void set_buffer_text(Editor *editor, const std::string &value)
    {
        Editor *tab = document(editor);
        tab->loading = true;
        gtk_text_buffer_set_text(text_buffer(editor), value.c_str(), -1);
        tab->loading = false;
    }

    void on_buffer_changed(GtkTextBuffer *, Editor *editor)
    {
        if (document(editor)->loading)
            return;

        document(editor)->dirty = true;
        update_window_title(editor);
        update_position_status(editor);
    }

    void on_cursor_moved(GtkTextBuffer *, GtkTextIter *, GtkTextMark *, Editor *editor)
    {
        update_position_status(editor);
    }

    bool save_document(Editor *editor, bool choose_path);
    Editor *create_tab(Editor *editor);
    void on_new_document(GtkWidget *, Editor *editor);
    void on_open_document(GtkWidget *, Editor *editor);
    void on_save_document(GtkWidget *, Editor *editor);
    void on_save_as(GtkWidget *, Editor *editor);
    void on_start_engine(GtkWidget *, Editor *editor);
    void on_help(GtkWidget *, Editor *editor);
    void on_toolbar_toggled(GtkCheckMenuItem *, Editor *editor);
    void on_statusbar_toggled(GtkCheckMenuItem *, Editor *editor);
    void find_text(Editor *editor);
    gboolean on_key_press(GtkWidget *, GdkEventKey *, Editor *);
    gboolean on_scroll(GtkWidget *, GdkEventScroll *, Editor *);
    void on_buffer_changed(GtkTextBuffer *, Editor *);
    void on_cursor_moved(GtkTextBuffer *, GtkTextIter *, GtkTextMark *, Editor *);
    void on_switch_page(GtkNotebook *, GtkWidget *, guint, Editor *);

    Editor *create_tab(Editor *app)
    {
        Editor *tab = new Editor{};
        tab->workspace = app;
        tab->view = gtk_text_view_new();
        gtk_widget_set_name(tab->view, "nudi-editor");
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->view), GTK_WRAP_WORD_CHAR);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(tab->view), FALSE);
        gtk_widget_set_vexpand(tab->view, TRUE);

        GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), tab->view);

        tab->tab_label = gtk_label_new("Untitled");
        tab->page = scrolled;
        app->active_tab = tab;
        g_object_set_data_full(
            G_OBJECT(scrolled),
            "nudi-document",
            tab,
            [](gpointer data)
            { delete static_cast<Editor *>(data); });

        gtk_notebook_append_page(
            GTK_NOTEBOOK(app->notebook),
            scrolled,
            tab->tab_label);
        gtk_notebook_set_tab_reorderable(
            GTK_NOTEBOOK(app->notebook),
            scrolled,
            TRUE);

        g_signal_connect(tab->view, "key-press-event", G_CALLBACK(on_key_press), app);
        g_signal_connect(tab->view, "scroll-event", G_CALLBACK(on_scroll), app);
        g_signal_connect(text_buffer(tab), "changed", G_CALLBACK(on_buffer_changed), tab);
        g_signal_connect(
            text_buffer(tab),
            "mark-set",
            G_CALLBACK(on_cursor_moved),
            tab);

        gtk_notebook_set_current_page(
            GTK_NOTEBOOK(app->notebook),
            gtk_notebook_page_num(GTK_NOTEBOOK(app->notebook), scrolled));
        apply_zoom(tab);
        update_window_title(tab);
        update_position_status(tab);
        gtk_widget_show_all(scrolled);
        gtk_widget_grab_focus(tab->view);
        return tab;
    }

    Editor *create_help_tab(Editor *app)
    {
        if (app->help_tab != nullptr)
        {
            gtk_notebook_set_current_page(
                GTK_NOTEBOOK(app->notebook),
                gtk_notebook_page_num(
                    GTK_NOTEBOOK(app->notebook),
                    app->help_tab->page));
            return app->help_tab;
        }

        Editor *tab = new Editor{};
        tab->workspace = app;
        tab->help = true;
        tab->view = gtk_text_view_new();
        gtk_widget_set_name(tab->view, "nudi-editor");
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->view), GTK_WRAP_WORD_CHAR);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tab->view), FALSE);
        gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tab->view), 18);
        gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tab->view), 18);
        gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tab->view), 14);
        gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tab->view), 14);

        const char *help_text =
            "Nudi Kannada\n"
            "================\n\n"
            "A Kannada Nudi keyboard and text editor for Ubuntu.\n\n"
            "Keyboard engine\n"
            "• Type using the Nudi keyboard mappings.\n"
            "• Use Space to commit the current composition.\n"
            "• Use Ctrl + mouse wheel or View > Zoom to change text size.\n\n"
            "Editor\n"
            "• File > New creates a new tab.\n"
            "• File > Open opens a text file in a new tab.\n"
            "• File > Save saves the active tab.\n"
            "• Edit > Find searches the active document.\n\n"
            "Developed and maintained by Kagapa\n"
            "Support: kagapa@gmail.com\n";
        gtk_text_buffer_set_text(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->view)),
            help_text,
            -1);

        GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(scrolled),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), tab->view);

        tab->tab_label = gtk_label_new("Help");
        tab->page = scrolled;
        app->help_tab = tab;
        app->active_tab = tab;
        g_object_set_data_full(
            G_OBJECT(scrolled),
            "nudi-document",
            tab,
            [](gpointer data)
            { delete static_cast<Editor *>(data); });

        gtk_notebook_append_page(
            GTK_NOTEBOOK(app->notebook),
            scrolled,
            tab->tab_label);
        gtk_notebook_set_tab_reorderable(
            GTK_NOTEBOOK(app->notebook),
            scrolled,
            TRUE);
        gtk_notebook_set_current_page(
            GTK_NOTEBOOK(app->notebook),
            gtk_notebook_page_num(GTK_NOTEBOOK(app->notebook), scrolled));
        apply_zoom(tab);
        update_window_title(tab);
        update_position_status(tab);
        gtk_widget_show_all(scrolled);
        gtk_widget_grab_focus(tab->view);
        return tab;
    }

    void on_switch_page(
        GtkNotebook *,
        GtkWidget *page,
        guint,
        Editor *app)
    {
        Editor *tab = static_cast<Editor *>(
            g_object_get_data(G_OBJECT(page), "nudi-document"));
        if (tab == nullptr)
            return;

        app->active_tab = tab;
        update_window_title(tab);
        update_position_status(tab);
        apply_zoom(tab);
        gtk_widget_grab_focus(tab->view);
    }

    void on_help(GtkWidget *, Editor *editor)
    {
        create_help_tab(workspace(editor));
    }

    void new_document(Editor *editor)
    {
        create_tab(workspace(editor));
    }

    void open_document(Editor *editor)
    {
        GtkWidget *dialog = gtk_file_chooser_dialog_new(
            "Open Text File",
            GTK_WINDOW(workspace(editor)->window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Open", GTK_RESPONSE_ACCEPT,
            nullptr);

        GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "Text files");
        gtk_file_filter_add_pattern(filter, "*.txt");
        gtk_file_filter_add_pattern(filter, "*.text");
        gtk_file_filter_add_pattern(filter, "*");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        {
            gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gchar *contents = nullptr;
            gsize length = 0;
            GError *error = nullptr;

            if (path != nullptr &&
                g_file_get_contents(path, &contents, &length, &error))
            {
                std::string value(contents, length);
                if (!g_utf8_validate(value.c_str(), value.size(), nullptr))
                    value = g_utf8_make_valid(value.c_str(), value.size());

                Editor *tab = create_tab(workspace(editor));
                set_buffer_text(tab, value);
                tab->filename = path;
                tab->line_ending =
                    value.find("\r\n") != std::string::npos ? "CRLF" : "LF";
                tab->dirty = false;
                update_window_title(tab);
                update_position_status(tab);
                update_status(tab, "File opened");
            }
            else
            {
                const std::string message =
                    error != nullptr ? error->message : "Unable to open file";
                update_status(editor, message.c_str());
            }

            if (error != nullptr)
                g_error_free(error);
            g_free(contents);
            g_free(path);
        }

        gtk_widget_destroy(dialog);
    }

    bool save_document(Editor *editor, bool choose_path)
    {
        Editor *tab = document(editor);
        Editor *app = workspace(editor);

        if (tab->help)
        {
            update_status(app, "Help tab is read-only");
            return false;
        }

        if (choose_path || tab->filename.empty())
        {
            GtkWidget *dialog = gtk_file_chooser_dialog_new(
                "Save Text File",
                GTK_WINDOW(app->window),
                GTK_FILE_CHOOSER_ACTION_SAVE,
                "Cancel", GTK_RESPONSE_CANCEL,
                "Save", GTK_RESPONSE_ACCEPT,
                nullptr);

            gtk_file_chooser_set_do_overwrite_confirmation(
                GTK_FILE_CHOOSER(dialog), TRUE);
            gtk_file_chooser_set_current_name(
                GTK_FILE_CHOOSER(dialog), "untitled.txt");

            GtkFileFilter *filter = gtk_file_filter_new();
            gtk_file_filter_set_name(filter, "Text files");
            gtk_file_filter_add_pattern(filter, "*.txt");
            gtk_file_filter_add_pattern(filter, "*.text");
            gtk_file_filter_add_pattern(filter, "*");
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

            if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT)
            {
                gtk_widget_destroy(dialog);
                return false;
            }

            gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            tab->filename = path != nullptr ? path : "";
            g_free(path);
            gtk_widget_destroy(dialog);
        }

        GError *error = nullptr;
        const std::string value = buffer_text(editor);
        const gboolean saved = g_file_set_contents(
            tab->filename.c_str(), value.data(), value.size(), &error);

        if (!saved)
        {
            const std::string message =
                error != nullptr ? error->message : "Unable to save file";
            update_status(editor, message.c_str());
            if (error != nullptr)
                g_error_free(error);
            return false;
        }

        tab->dirty = false;
        update_window_title(editor);
        update_status(editor, "File saved");
        return true;
    }

    gboolean on_scroll(GtkWidget *, GdkEventScroll *event, Editor *editor)
    {
        Editor *tab = document(editor);
        if ((event->state & GDK_CONTROL_MASK) == 0)
            return FALSE;

        if (event->direction == GDK_SCROLL_UP || event->delta_y < 0)
            tab->zoom = std::min(tab->zoom + 1, 40);
        else if (event->direction == GDK_SCROLL_DOWN || event->delta_y > 0)
            tab->zoom = std::max(tab->zoom - 1, 8);
        else
            return TRUE;

        apply_zoom(editor);
        return TRUE;
    }

    void zoom_in(Editor *editor)
    {
        Editor *tab = document(editor);
        tab->zoom = std::min(tab->zoom + 1, 40);
        apply_zoom(editor);
    }

    void zoom_out(Editor *editor)
    {
        Editor *tab = document(editor);
        tab->zoom = std::max(tab->zoom - 1, 8);
        apply_zoom(editor);
    }

    void zoom_reset(Editor *editor)
    {
        document(editor)->zoom = 14;
        apply_zoom(editor);
    }

    void cut_text(Editor *editor)
    {
        gtk_text_buffer_cut_clipboard(
            text_buffer(editor),
            gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
            TRUE);
    }

    void copy_text(Editor *editor)
    {
        gtk_text_buffer_copy_clipboard(
            text_buffer(editor),
            gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    }

    void paste_text(Editor *editor)
    {
        gtk_text_buffer_paste_clipboard(
            text_buffer(editor),
            gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
            nullptr,
            TRUE);
    }

    void select_all(Editor *editor)
    {
        GtkTextBuffer *buffer = text_buffer(editor);
        GtkTextIter start;
        GtkTextIter end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_select_range(buffer, &start, &end);
    }

    void on_cut(GtkWidget *, Editor *editor)
    {
        cut_text(editor);
    }

    void on_copy(GtkWidget *, Editor *editor)
    {
        copy_text(editor);
    }

    void on_paste(GtkWidget *, Editor *editor)
    {
        paste_text(editor);
    }

    void on_select_all(GtkWidget *, Editor *editor)
    {
        select_all(editor);
    }

    void on_find(GtkWidget *, Editor *editor)
    {
        find_text(editor);
    }

    void on_zoom_in_menu(GtkWidget *, Editor *editor)
    {
        zoom_in(editor);
    }

    void on_zoom_out_menu(GtkWidget *, Editor *editor)
    {
        zoom_out(editor);
    }

    void on_zoom_reset_menu(GtkWidget *, Editor *editor)
    {
        zoom_reset(editor);
    }

    void on_wrap_toggled(GtkCheckMenuItem *item, Editor *editor)
    {
        gtk_text_view_set_wrap_mode(
            GTK_TEXT_VIEW(document(editor)->view),
            gtk_check_menu_item_get_active(item)
                ? GTK_WRAP_WORD_CHAR
                : GTK_WRAP_NONE);
    }

    void on_exit_menu(GtkWidget *, Editor *editor)
    {
        gtk_window_close(GTK_WINDOW(workspace(editor)->window));
    }

    void on_toolbar_new(GtkToolButton *, Editor *editor)
    {
        new_document(editor);
    }

    void on_toolbar_open(GtkToolButton *, Editor *editor)
    {
        open_document(editor);
    }

    void on_toolbar_save(GtkToolButton *, Editor *editor)
    {
        save_document(editor, false);
    }

    void on_toolbar_find(GtkToolButton *, Editor *editor)
    {
        find_text(editor);
    }

    void on_toolbar_zoom_out(GtkToolButton *, Editor *editor)
    {
        zoom_out(editor);
    }

    void on_toolbar_zoom_reset(GtkToolButton *, Editor *editor)
    {
        zoom_reset(editor);
    }

    void on_toolbar_zoom_in(GtkToolButton *, Editor *editor)
    {
        zoom_in(editor);
    }

    GtkToolItem *toolbar_button(
        GtkToolbar *toolbar,
        const char *icon_name,
        const char *tooltip,
        GCallback callback,
        Editor *editor)
    {
        GtkToolItem *button = gtk_tool_button_new(
            gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR),
            nullptr);
        gtk_tool_item_set_tooltip_text(button, tooltip);
        gtk_toolbar_insert(toolbar, button, -1);
        g_signal_connect(button, "clicked", callback, editor);
        return button;
    }

    GtkWidget *create_toolbar(Editor *editor)
    {
        GtkWidget *toolbar = gtk_toolbar_new();
        gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
        gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_widget_set_margin_top(toolbar, 2);
        gtk_widget_set_margin_bottom(toolbar, 2);

        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "document-new",
            "New tab (Ctrl+N)",
            G_CALLBACK(on_toolbar_new),
            editor);
        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "document-open",
            "Open text file (Ctrl+O)",
            G_CALLBACK(on_toolbar_open),
            editor);
        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "document-save",
            "Save document (Ctrl+S)",
            G_CALLBACK(on_toolbar_save),
            editor);

        GtkToolItem *separator = gtk_separator_tool_item_new();
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), separator, -1);
        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "edit-find",
            "Find text (Ctrl+F)",
            G_CALLBACK(on_toolbar_find),
            editor);

        separator = gtk_separator_tool_item_new();
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), separator, -1);
        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "zoom-out",
            "Zoom out",
            G_CALLBACK(on_toolbar_zoom_out),
            editor);
        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "zoom-original",
            "Reset zoom",
            G_CALLBACK(on_toolbar_zoom_reset),
            editor);
        toolbar_button(
            GTK_TOOLBAR(toolbar),
            "zoom-in",
            "Zoom in",
            G_CALLBACK(on_toolbar_zoom_in),
            editor);

        editor->toolbar = toolbar;
        return toolbar;
    }

    void on_toolbar_toggled(GtkCheckMenuItem *item, Editor *editor)
    {
        gtk_widget_set_visible(
            workspace(editor)->toolbar,
            gtk_check_menu_item_get_active(item));
    }

    void on_statusbar_toggled(GtkCheckMenuItem *item, Editor *editor)
    {
        gtk_widget_set_visible(
            workspace(editor)->status_bar,
            gtk_check_menu_item_get_active(item));
    }

    GtkWidget *menu_item(
        GtkWidget *menu,
        const char *label,
        GCallback callback,
        Editor *editor)
    {
        GtkWidget *item = gtk_menu_item_new_with_mnemonic(label);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(item, "activate", callback, editor);
        return item;
    }

    GtkWidget *create_menu_bar(Editor *editor)
    {
        GtkWidget *bar = gtk_menu_bar_new();

        GtkWidget *file = gtk_menu_item_new_with_mnemonic("_File");
        GtkWidget *file_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(bar), file);
        menu_item(file_menu, "_New", G_CALLBACK(on_new_document), editor);
        menu_item(file_menu, "_Open", G_CALLBACK(on_open_document), editor);
        menu_item(file_menu, "_Save", G_CALLBACK(on_save_document), editor);
        menu_item(file_menu, "Save _As", G_CALLBACK(on_save_as), editor);
        GtkWidget *separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), separator);
        menu_item(file_menu, "E_xit", G_CALLBACK(on_exit_menu), editor);

        GtkWidget *edit = gtk_menu_item_new_with_mnemonic("_Edit");
        GtkWidget *edit_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit), edit_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(bar), edit);
        menu_item(edit_menu, "Cu_t", G_CALLBACK(on_cut), editor);
        menu_item(edit_menu, "_Copy", G_CALLBACK(on_copy), editor);
        menu_item(edit_menu, "_Paste", G_CALLBACK(on_paste), editor);
        menu_item(edit_menu, "Select _All", G_CALLBACK(on_select_all), editor);
        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), separator);
        menu_item(edit_menu, "_Find", G_CALLBACK(on_find), editor);

        GtkWidget *view = gtk_menu_item_new_with_mnemonic("_View");
        GtkWidget *view_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(view), view_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(bar), view);
        menu_item(view_menu, "Zoom _In", G_CALLBACK(on_zoom_in_menu), editor);
        menu_item(view_menu, "Zoom _Out", G_CALLBACK(on_zoom_out_menu), editor);
        menu_item(view_menu, "_Reset Zoom", G_CALLBACK(on_zoom_reset_menu), editor);
        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), separator);
        GtkWidget *wrap = gtk_check_menu_item_new_with_label("Word Wrap");
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wrap), TRUE);
        gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), wrap);
        g_signal_connect(wrap, "toggled", G_CALLBACK(on_wrap_toggled), editor);

        GtkWidget *engine = gtk_menu_item_new_with_mnemonic("_Engine");
        GtkWidget *engine_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(engine), engine_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(bar), engine);
        menu_item(
            engine_menu,
            "Start _Nudi Engine",
            G_CALLBACK(on_start_engine),
            editor);

        GtkWidget *help = gtk_menu_item_new_with_mnemonic("_Help");
        GtkWidget *help_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), help_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(bar), help);
        menu_item(help_menu, "_Nudi Help", G_CALLBACK(on_help), editor);

        GtkWidget *options = gtk_menu_item_new_with_mnemonic("_Options");
        GtkWidget *options_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(options), options_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(bar), options);

        GtkWidget *toolbar_item =
            gtk_check_menu_item_new_with_label("Show Toolbar");
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(toolbar_item), TRUE);
        gtk_menu_shell_append(GTK_MENU_SHELL(options_menu), toolbar_item);
        g_signal_connect(
            toolbar_item,
            "toggled",
            G_CALLBACK(on_toolbar_toggled),
            editor);

        GtkWidget *status_item =
            gtk_check_menu_item_new_with_label("Show Status Bar");
        gtk_check_menu_item_set_active(
            GTK_CHECK_MENU_ITEM(status_item), TRUE);
        gtk_menu_shell_append(GTK_MENU_SHELL(options_menu), status_item);
        g_signal_connect(
            status_item,
            "toggled",
            G_CALLBACK(on_statusbar_toggled),
            editor);

        return bar;
    }

    void find_text(Editor *editor)
    {
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Find",
            GTK_WINDOW(workspace(editor)->window),
            GTK_DIALOG_MODAL,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Find Next", GTK_RESPONSE_ACCEPT,
            nullptr);
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Search text");
        gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
        gtk_widget_show_all(dialog);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        {
            const gchar *query = gtk_entry_get_text(GTK_ENTRY(entry));
            if (query != nullptr && *query != '\0')
            {
                GtkTextBuffer *buffer = text_buffer(editor);
                GtkTextIter start;
                GtkTextIter match_start;
                GtkTextIter match_end;
                gtk_text_buffer_get_iter_at_mark(
                    buffer,
                    &start,
                    gtk_text_buffer_get_insert(buffer));

                bool found = gtk_text_iter_forward_search(
                    &start,
                    query,
                    GTK_TEXT_SEARCH_TEXT_ONLY,
                    &match_start,
                    &match_end,
                    nullptr);

                if (!found)
                {
                    gtk_text_buffer_get_start_iter(buffer, &start);
                    found = gtk_text_iter_forward_search(
                        &start,
                        query,
                        GTK_TEXT_SEARCH_TEXT_ONLY,
                        &match_start,
                        &match_end,
                        nullptr);
                }

                if (found)
                {
                    gtk_text_buffer_select_range(
                        buffer,
                        &match_start,
                        &match_end);
                    gtk_text_view_scroll_to_iter(
                        GTK_TEXT_VIEW(document(editor)->view),
                        &match_start,
                        0.15,
                        FALSE,
                        0.0,
                        0.5);
                }
                else
                {
                    update_status(editor, "Text not found");
                }
            }
        }

        gtk_widget_destroy(dialog);
    }

    gboolean on_key_press(GtkWidget *, GdkEventKey *event, Editor *editor)
    {
        const bool control =
            (event->state & GDK_CONTROL_MASK) != 0;
        const bool alternate =
            (event->state & GDK_MOD1_MASK) != 0;

        // Keep document commands available while IBus owns normal input.
        if (control && !alternate)
        {
            switch (event->keyval)
            {
            case GDK_KEY_n:
            case GDK_KEY_N:
                new_document(editor);
                return TRUE;
            case GDK_KEY_o:
            case GDK_KEY_O:
                open_document(editor);
                return TRUE;
            case GDK_KEY_s:
            case GDK_KEY_S:
                save_document(editor, (event->state & GDK_SHIFT_MASK) != 0);
                return TRUE;
            default:
                break;
            }
        }

        // When IBus is active it owns the composition and commits text
        // directly to the GtkTextView.  Do not run the editor's separate
        // local composer or refresh the buffer, or a separator such as
        // Space would overwrite IBus' committed/preedit text.
        // GTK/IBus handles normal text editing, including selection,
        // backspace, cursor movement, copy/paste, undo/redo, and Space.
        return FALSE;
    }

    std::string trim_copy(const std::string &value)
    {
        const std::size_t start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
        {
            return {};
        }

        const std::size_t end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    bool run_ibus_command(const std::string &command, std::string *output = nullptr)
    {
        GError *error = nullptr;
        gint exit_status = 0;
        gchar *stdout = nullptr;
        gchar *stderr = nullptr;

        const gboolean ok = g_spawn_command_line_sync(
            command.c_str(),
            &stdout,
            &stderr,
            &exit_status,
            &error);

        if (stdout != nullptr)
        {
            if (output != nullptr)
            {
                *output = stdout;
            }
            g_free(stdout);
        }

        if (stderr != nullptr)
        {
            g_free(stderr);
        }

        if (error != nullptr)
        {
            g_error_free(error);
            return false;
        }

        return ok && exit_status == 0;
    }

    bool wait_for_ibus_ready(int retries)
    {
        for (int attempt = 0; attempt < retries; ++attempt)
        {
            std::string output;
            if (run_ibus_command("ibus list-engine", &output))
            {
                return true;
            }
            g_usleep(250000);
        }

        return false;
    }

    bool ibus_has_nudi_engine()
    {
        std::string output;
        if (!run_ibus_command("ibus list-engine", &output))
        {
            return false;
        }

        return output.find("nudi") != std::string::npos;
    }

    bool is_active_engine_nudi()
    {
        std::string output;
        if (!run_ibus_command("ibus engine", &output))
        {
            return false;
        }

        return trim_copy(output) == "nudi";
    }

    bool ensure_ibus_session(Editor *editor)
    {
        if (editor != nullptr)
        {
            editor->ibus_active = false;
            update_status(editor, "Checking IBus...");
        }

        if (!ibus_has_nudi_engine())
        {
            if (!run_ibus_command("ibus-daemon --panel disable --xim --daemonize --replace"))
            {
                if (editor != nullptr)
                {
                    update_status(editor, "Failed to start IBus");
                }
                return false;
            }

            if (!wait_for_ibus_ready(20))
            {
                if (editor != nullptr)
                {
                    update_status(editor, "IBus is still unavailable");
                }
                return false;
            }
        }

        if (!is_active_engine_nudi())
        {
            if (!run_ibus_command("ibus engine nudi"))
            {
                if (editor != nullptr)
                {
                    update_status(editor, "Nudi could not be activated");
                }
                return false;
            }
        }

        if (editor != nullptr)
        {
            editor->ibus_active = true;
            update_status(editor, "INS  · Nudi active");
        }

        return true;
    }

    void shutdown_ibus_session()
    {
        if (editor_window_count() > 0)
        {
            return;
        }

        run_ibus_command("ibus exit");
    }

    void on_start_engine(GtkWidget *, Editor *editor)
    {
        ensure_ibus_session(editor);
    }

    void on_new_document(GtkWidget *, Editor *editor)
    {
        new_document(editor);
    }

    void on_open_document(GtkWidget *, Editor *editor)
    {
        open_document(editor);
    }

    void on_save_document(GtkWidget *, Editor *editor)
    {
        save_document(editor, false);
    }

    void on_save_as(GtkWidget *, Editor *editor)
    {
        save_document(editor, true);
    }

    bool confirm_close(Editor *editor)
    {
        Editor *app = workspace(editor);
        GtkNotebook *notebook = GTK_NOTEBOOK(app->notebook);
        const gint original_page = gtk_notebook_get_current_page(notebook);
        const gint page_count = gtk_notebook_get_n_pages(notebook);

        for (gint page_number = 0; page_number < page_count; ++page_number)
        {
            GtkWidget *page = gtk_notebook_get_nth_page(notebook, page_number);
            Editor *tab = static_cast<Editor *>(
                g_object_get_data(G_OBJECT(page), "nudi-document"));
            if (tab == nullptr || !tab->dirty)
                continue;

            app->active_tab = tab;
            gtk_notebook_set_current_page(notebook, page_number);

            GtkWidget *dialog = gtk_message_dialog_new(
                GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_WARNING,
                GTK_BUTTONS_NONE,
                "Save changes to %s before closing?",
                tab->filename.empty() ? "Untitled" : tab->filename.c_str());
            gtk_dialog_add_buttons(
                GTK_DIALOG(dialog),
                "Cancel", GTK_RESPONSE_CANCEL,
                "Discard", GTK_RESPONSE_NO,
                "Save", GTK_RESPONSE_YES,
                nullptr);

            const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);

            if (response == GTK_RESPONSE_CANCEL)
                return false;
            if (response == GTK_RESPONSE_YES && !save_document(tab, false))
                return false;
        }

        if (original_page >= 0 && original_page < page_count)
            gtk_notebook_set_current_page(notebook, original_page);
        return true;
    }

    gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *, Editor *editor)
    {
        if (!confirm_close(editor))
            return TRUE;

        if (editor_window_count() > 0)
        {
            editor_window_count()--;
        }

        if (editor_window_count() <= 0)
        {
            editor_window_count() = 0;
            shutdown_ibus_session();
        }

        gtk_widget_destroy(widget);
        return TRUE;
    }

    void on_activate(GtkApplication *application, gpointer)
    {
        Editor *editor = new Editor{};
        editor->window = gtk_application_window_new(application);
        gtk_window_set_title(GTK_WINDOW(editor->window), "Nudi Kannada Editor");
        gtk_window_set_icon_name(GTK_WINDOW(editor->window), "nudi");
        gtk_window_set_default_size(GTK_WINDOW(editor->window), 720, 460);

        GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(layout), 16);
        gtk_container_add(GTK_CONTAINER(editor->window), layout);

        gtk_box_pack_start(
            GTK_BOX(layout),
            create_menu_bar(editor),
            FALSE,
            FALSE,
            0);

        gtk_box_pack_start(
            GTK_BOX(layout),
            create_toolbar(editor),
            FALSE,
            FALSE,
            0);

        editor->notebook = gtk_notebook_new();
        gtk_notebook_set_scrollable(GTK_NOTEBOOK(editor->notebook), TRUE);
        gtk_notebook_set_show_border(GTK_NOTEBOOK(editor->notebook), FALSE);
        gtk_widget_set_vexpand(editor->notebook, TRUE);
        gtk_box_pack_start(GTK_BOX(layout), editor->notebook, TRUE, TRUE, 0);

        GtkWidget *plus = gtk_button_new();
        gtk_button_set_image(
            GTK_BUTTON(plus),
            gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON));
        gtk_button_set_relief(GTK_BUTTON(plus), GTK_RELIEF_NONE);
        gtk_widget_set_tooltip_text(plus, "New tab");
        gtk_notebook_set_action_widget(
            GTK_NOTEBOOK(editor->notebook),
            plus,
            GTK_PACK_END);
        g_signal_connect(plus, "clicked", G_CALLBACK(on_new_document), editor);

        editor->status = gtk_label_new("Ln 1, Col 1  |  0 chars");
        gtk_widget_set_halign(editor->status, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(editor->status), 0.0F);

        editor->document_status = gtk_label_new("LF · UTF-8    INS");
        gtk_label_set_xalign(GTK_LABEL(editor->document_status), 1.0F);

        editor->status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_top(editor->status_bar, 4);
        gtk_box_pack_start(
            GTK_BOX(editor->status_bar),
            editor->status,
            TRUE,
            TRUE,
            0);
        gtk_box_pack_end(
            GTK_BOX(editor->status_bar),
            editor->document_status,
            FALSE,
            FALSE,
            0);
        gtk_box_pack_start(
            GTK_BOX(layout),
            editor->status_bar,
            FALSE,
            FALSE,
            0);

        g_signal_connect(
            editor->notebook,
            "switch-page",
            G_CALLBACK(on_switch_page),
            editor);
        g_signal_connect(editor->window, "delete-event", G_CALLBACK(on_window_delete_event), editor);
        g_object_set_data_full(G_OBJECT(editor->window), "nudi-editor", editor,
                               [](gpointer data)
                               { delete static_cast<Editor *>(data); });

        editor_window_count()++;
        create_tab(editor);
        ensure_ibus_session(editor);

        gtk_widget_show_all(editor->window);
    }

} // namespace

int main(int argc, char **argv)
{
    GtkApplication *application = gtk_application_new(
        "com.example.NudiEditor", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(on_activate), nullptr);
    const int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
