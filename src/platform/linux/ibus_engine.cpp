#include "nudi_composer.hpp"

#include <ibus.h>

#include <cctype>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include <glib.h>
#include <glib/gstdio.h>

#include <unistd.h>

typedef struct _NudiEngine
{
    IBusEngine parent;
    nudi::Composer *composer;
} NudiEngine;

typedef struct _NudiEngineClass
{
    IBusEngineClass parent_class;
} NudiEngineClass;

namespace
{

    static void nudi_engine_init(NudiEngine *engine);
    static void nudi_engine_class_init(NudiEngineClass *klass);

    G_DEFINE_TYPE(NudiEngine, nudi_engine, IBUS_TYPE_ENGINE)

    // ============================================================
    // Logging
    // ============================================================
    //
    // Use the user's XDG state directory instead of /var/log.
    //
    // This avoids requiring the engine to run as root and avoids
    // creating a world-writable /var/log directory.
    //
    // Example:
    //   ~/.local/state/kannada-nudi/2026-08-29/engine-1000.log
    //
    // ============================================================

    std::string log_path()
    {
        const char *state_home = g_get_user_state_dir();

        if (state_home == nullptr || *state_home == '\0')
        {
            return {};
        }

        const std::time_t now = std::time(nullptr);

        std::tm local_time{};
        localtime_r(&now, &local_time);

        std::ostringstream date;
        date << std::put_time(&local_time, "%Y-%m-%d");

        const std::string directory =
            std::string(state_home) +
            "/kannada-nudi/" +
            date.str();

        if (g_mkdir_with_parents(directory.c_str(), 0700) != 0)
        {
            return {};
        }

        return directory +
               "/engine-" +
               std::to_string(static_cast<unsigned long>(getuid())) +
               ".log";
    }

    void log_event(const std::string &event)
    {
        const std::string path = log_path();

        if (path.empty())
        {
            return;
        }

        std::ofstream log(path, std::ios::app);

        if (!log)
        {
            return;
        }

        const std::time_t now = std::time(nullptr);

        std::tm local_time{};
        localtime_r(&now, &local_time);

        log << std::put_time(
                   &local_time,
                   "%Y-%m-%dT%H:%M:%S%z")
            << " "
            << event
            << '\n';
    }

    // ============================================================
    // IBus component registration
    // ============================================================

    void register_component(IBusBus *bus)
    {
        if (bus == nullptr)
        {
            return;
        }

        static const char *candidates[] = {
            "/usr/share/ibus/component/com.kagapa.Nudi.xml",
            "/usr/local/share/ibus/component/com.kagapa.Nudi.xml",

            // Useful while running directly from the build directory.
            "./com.kagapa.Nudi.xml",

            nullptr};

        for (std::size_t i = 0; candidates[i] != nullptr; ++i)
        {

            if (!g_file_test(
                    candidates[i],
                    G_FILE_TEST_EXISTS))
            {
                continue;
            }

            IBusComponent *component =
                ibus_component_new_from_file(candidates[i]);

            if (component == nullptr)
            {
                log_event(
                    std::string("component_load_failed path=") +
                    candidates[i]);

                continue;
            }

            ibus_bus_register_component(
                bus,
                component);

            log_event(
                std::string("component_registered path=") +
                candidates[i]);

            g_object_unref(component);

            return;
        }

        g_printerr(
            "Unable to locate IBus component metadata for Nudi.\n");

        log_event("component_metadata_missing");
    }

    // ============================================================
    // Update IBus preedit
    // ============================================================

    void update_preedit(NudiEngine *engine)
    {
        if (engine == nullptr ||
            engine->composer == nullptr)
        {
            return;
        }

        const std::string &value =
            engine->composer->preedit();

        IBusText *text =
            ibus_text_new_from_string(
                value.c_str());

        const guint cursor_position =
            static_cast<guint>(
                g_utf8_strlen(
                    value.c_str(),
                    -1));

        ibus_engine_update_preedit_text(
            IBUS_ENGINE(engine),
            text,
            cursor_position,
            !value.empty());

        g_object_unref(text);
    }

    // ============================================================
    // Commit text to the focused application
    // ============================================================

    void commit_text(
        IBusEngine *ibus_engine,
        const std::string &value)
    {
        if (ibus_engine == nullptr ||
            value.empty())
        {
            return;
        }

        IBusText *text =
            ibus_text_new_from_string(
                value.c_str());

        ibus_engine_commit_text(
            ibus_engine,
            text);

        g_object_unref(text);
    }

    // ============================================================
    // Reset
    // ============================================================

    static void nudi_engine_reset(
        IBusEngine *ibus_engine)
    {
        NudiEngine *engine =
            reinterpret_cast<NudiEngine *>(
                ibus_engine);

        if (engine == nullptr ||
            engine->composer == nullptr)
        {
            return;
        }

        engine->composer->reset();

        update_preedit(engine);

        log_event("reset");

        IBusEngineClass *parent_class =
            IBUS_ENGINE_CLASS(
                nudi_engine_parent_class);

        if (parent_class->reset != nullptr)
        {
            parent_class->reset(ibus_engine);
        }
    }

    // ============================================================
    // Process key event
    // ============================================================

    static gboolean nudi_engine_process_key_event(
        IBusEngine *ibus_engine,
        guint keyval,
        guint keycode,
        guint state)
    {
        NudiEngine *engine =
            reinterpret_cast<NudiEngine *>(
                ibus_engine);

        if (engine == nullptr ||
            engine->composer == nullptr)
        {
            return FALSE;
        }

        // ========================================================
        // Ignore key release events
        // ========================================================

        if ((state & IBUS_RELEASE_MASK) != 0)
        {
            return FALSE;
        }

        // ========================================================
        // Modifier states
        // ========================================================

        const bool shifted =
            (state & IBUS_SHIFT_MASK) != 0;

        const bool caps_lock =
            (state & IBUS_LOCK_MASK) != 0;

        const bool alt =
            (state & IBUS_MOD1_MASK) != 0;

        const bool num_lock =
            (state & IBUS_MOD2_MASK) != 0;

        const bool scroll_lock =
            (state & IBUS_MOD3_MASK) != 0;

        // ========================================================
        // Ignore Control / Super combinations
        // ========================================================

        const guint blocked =
            IBUS_CONTROL_MASK |
            IBUS_SUPER_MASK;

        if ((state & blocked) != 0)
        {
            return FALSE;
        }

        // ========================================================
        // Alt handling
        //
        // The Composer currently only supports Alt for its
        // Caps+Shift special mappings.
        //
        // Everything else goes to the application.
        // ========================================================

        if (
            alt &&
            !(caps_lock && shifted))
        {
            return FALSE;
        }

        // ========================================================
        // Let lock-key events pass through.
        //
        // We use their state but do not compose them.
        // ========================================================

        if (
            keyval == IBUS_KEY_Caps_Lock ||
            keyval == IBUS_KEY_Num_Lock ||
            keyval == IBUS_KEY_Scroll_Lock)
        {
            return FALSE;
        }

        // ========================================================
        // Backspace
        // ========================================================

        if (keyval == IBUS_KEY_BackSpace)
        {

            if (!engine->composer->backspace())
            {
                return FALSE;
            }

            update_preedit(engine);

            return TRUE;
        }

        // ========================================================
        // Delete
        // ========================================================

        if (keyval == IBUS_KEY_Delete)
        {

            if (engine->composer->empty())
            {
                return FALSE;
            }

            engine->composer->reset();

            update_preedit(engine);

            log_event("delete_preedit");

            return TRUE;
        }

        // ========================================================
        // Escape
        //
        // Cancel current composition.
        // ========================================================

        if (keyval == IBUS_KEY_Escape)
        {

            if (engine->composer->empty())
            {
                return FALSE;
            }

            engine->composer->reset();

            update_preedit(engine);

            log_event("escape_cancel");

            return TRUE;
        }

        // ========================================================
        // Commit on separator keys
        // ========================================================

        const bool is_space =
            keyval == IBUS_KEY_space;

        const bool is_return =
            keyval == IBUS_KEY_Return;

        const bool is_kp_enter =
            keyval == IBUS_KEY_KP_Enter;

        const bool is_tab =
            keyval == IBUS_KEY_Tab;

        if (
            is_space ||
            is_return ||
            is_kp_enter ||
            is_tab)
        {
            if (!engine->composer->empty())
            {

                const std::string value =
                    engine->composer->separator();

                if (!value.empty())
                {
                    commit_text(
                        ibus_engine,
                        value);

                    log_event(
                        "commit_separator length=" +
                        std::to_string(value.size()));
                }

                update_preedit(engine);
            }

            // Return FALSE so the original separator
            // reaches the application.
            return FALSE;
        }

        // ========================================================
        // Only process printable ASCII keys.
        //
        // Nudi Composer currently works on ASCII key values.
        // ========================================================

        if (
            keyval < 0x20 ||
            keyval > 0x7e)
        {
            return FALSE;
        }

        // ========================================================
        // Convert ASCII key to lowercase.
        //
        // Shift state is passed separately to Composer.
        //
        // Example:
        //
        // physical 'K' with Shift
        //
        // key = 'k'
        // shifted = true
        //
        // ========================================================

        const char key =
            static_cast<char>(
                g_ascii_tolower(
                    static_cast<gchar>(keyval)));

        // ========================================================
        // Feed Composer
        // ========================================================

        const std::string committed =
            engine->composer->feed(
                key,
                shifted,
                caps_lock,
                scroll_lock,
                num_lock,
                alt);

        // ========================================================
        // Commit completed text
        // ========================================================

        if (!committed.empty())
        {

            commit_text(
                ibus_engine,
                committed);

            log_event(
                "commit length=" +
                std::to_string(
                    committed.size()));
        }

        // ========================================================
        // Update preedit
        // ========================================================

        update_preedit(engine);

        return TRUE;
    }

    // ============================================================
    // Finalize engine
    // ============================================================

    static void nudi_engine_finalize(
        GObject *object)
    {
        NudiEngine *engine =
            reinterpret_cast<NudiEngine *>(
                object);

        if (engine != nullptr)
        {

            delete engine->composer;
            engine->composer = nullptr;

            log_event("engine_finalized");
        }

        G_OBJECT_CLASS(
            nudi_engine_parent_class)
            ->finalize(object);
    }

    // ============================================================
    // Engine initialization
    // ============================================================

    static void nudi_engine_init(
        NudiEngine *engine)
    {
        engine->composer =
            new nudi::Composer();

        if (g_strcmp0(g_getenv("NUDI_DEFAULT_VOWEL"), "virama") == 0)
        {
            engine->composer->set_default_vowel(
                nudi::Composer::DefaultVowel::Virama);
        }

        log_event("engine_initialized");
    }

    // ============================================================
    // Engine class initialization
    // ============================================================

    static void nudi_engine_class_init(
        NudiEngineClass *klass)
    {
        IBusEngineClass *engine_class =
            IBUS_ENGINE_CLASS(klass);

        engine_class->process_key_event =
            nudi_engine_process_key_event;

        engine_class->reset =
            nudi_engine_reset;

        GObjectClass *object_class =
            G_OBJECT_CLASS(klass);

        object_class->finalize =
            nudi_engine_finalize;
    }

    // ============================================================
    // Main
    // ============================================================

} // namespace

int main()
{
    log_event("engine_start");

    // ========================================================
    // Initialize IBus
    // ========================================================

    ibus_init();

    // ========================================================
    // Connect to IBus daemon
    // ========================================================

    IBusBus *bus =
        ibus_bus_new();

    if (bus == nullptr)
    {

        log_event("ibus_bus_creation_failed");

        return 1;
    }

    if (!ibus_bus_is_connected(bus))
    {

        log_event(
            "ibus_connection_failed");

        g_object_unref(bus);

        return 1;
    }

    log_event("ibus_connected");

    // ========================================================
    // Register Nudi component
    // ========================================================

    register_component(bus);

    // ========================================================
    // Create IBus engine factory
    // ========================================================

    IBusFactory *factory =
        ibus_factory_new(
            ibus_bus_get_connection(bus));

    if (factory == nullptr)
    {

        log_event(
            "factory_creation_failed");

        g_object_unref(bus);

        return 1;
    }

    // ========================================================
    // Register "nudi" engine
    // ========================================================

    ibus_factory_add_engine(
        factory,
        "nudi",
        nudi_engine_get_type());

    log_event(
        "engine_registered name=nudi");

    // ========================================================
    // Enter IBus main loop
    // ========================================================

    ibus_main();

    // ========================================================
    // Cleanup
    // ========================================================

    log_event("engine_shutdown");

    g_object_unref(factory);
    g_object_unref(bus);

    return 0;
}
