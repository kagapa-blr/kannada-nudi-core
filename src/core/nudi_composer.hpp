#pragma once

#include <string>

namespace nudi
{

    class Composer
    {
    public:
        enum class DefaultVowel
        {
            A,
            Virama,
        };

        void set_default_vowel(DefaultVowel default_vowel)
        {
            default_vowel_ = default_vowel;
            reset();
        }

        DefaultVowel default_vowel() const
        {
            return default_vowel_;
        }

        // Current composition/preedit text.
        const std::string &preedit() const
        {
            return text_;
        }

        // Whether there is currently an active composition.
        bool empty() const
        {
            return text_.empty();
        }

        // Reset the current composition state.
        void reset();

        // Remove the last composed Unicode/code-point unit.
        bool backspace();

        // Finish the current composition.
        std::string separator();

        // Process one keyboard key.
        //
        // key:
        //     Physical/logical key represented as ASCII.
        //
        // shifted:
        //     Shift key state.
        //
        // caps_lock:
        //     Caps Lock state.
        //
        // scroll_lock:
        //     Scroll Lock state.
        //
        // num_lock:
        //     Num Lock state.
        //
        // alt:
        //     Alt key state.
        std::string feed(
            char key,
            bool shifted = false,
            bool caps_lock = false,
            bool scroll_lock = false,
            bool num_lock = false,
            bool alt = false);

    private:
        DefaultVowel default_vowel_ = DefaultVowel::A;

        // A consonant has been entered and can receive
        // a vowel sign or virama.
        bool consonant_pending_ = false;

        // A virama has been entered and the next consonant
        // may form a conjunct.
        bool virama_pending_ = false;

        // Distinguishes explicit f virama from the implicit virama
        // emitted by the null-vowel default mode.
        bool explicit_virama_ = false;

        // NumLock apostrophe alternates between the right and left
        // quotation marks, matching the Windows reference keyboard.
        bool apostrophe_toggle_ = false;

        // Current UTF-8 composition buffer.
        std::string text_;
    };

} // namespace nudi
