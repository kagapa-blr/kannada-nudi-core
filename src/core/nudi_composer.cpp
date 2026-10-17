#include "nudi_composer.hpp"

#include <cctype>
#include <cstddef>
#include <string>

namespace nudi
{
    namespace
    {

        // ============================================================
        // Unicode constants
        // ============================================================

        constexpr const char *virama = u8"\u0CCD";
        constexpr const char *zwnj = u8"\u200C";
        constexpr const char *zwj_virama = u8"\u200D\u0CCD";

        // ============================================================
        // Mapping
        // ============================================================

        struct Mapping
        {
            char key;
            const char *value;
        };

        // ============================================================
        // NUDI KANNADA CONSONANTS
        // ============================================================

        constexpr Mapping consonants[] = {
            {'k', u8"\u0C95"},
            {'g', u8"\u0C97"},
            {'c', u8"\u0C9A"},
            {'j', u8"\u0C9C"},

            {'q', u8"\u0C9F"},
            {'w', u8"\u0CA1"},

            {'t', u8"\u0CA4"},
            {'d', u8"\u0CA6"},

            {'p', u8"\u0CAA"},
            {'b', u8"\u0CAC"},

            {'y', u8"\u0CAF"},
            {'r', u8"\u0CB0"},
            {'l', u8"\u0CB2"},
            {'v', u8"\u0CB5"},

            {'s', u8"\u0CB8"},
            {'h', u8"\u0CB9"},

            {'x', u8"\u0CB7"},
            {'z', u8"\u0C9E"},

            {'n', u8"\u0CA8"},
            {'m', u8"\u0CAE"},
        };

        // ============================================================
        // NUDI SHIFT + CONSONANTS
        // ============================================================

        constexpr Mapping shifted_consonants[] = {
            {'k', u8"\u0C96"},
            {'g', u8"\u0C98"},

            {'c', u8"\u0C9B"},
            {'j', u8"\u0C9D"},

            {'q', u8"\u0CA0"},
            {'w', u8"\u0CA2"},

            {'t', u8"\u0CA5"},
            {'d', u8"\u0CA7"},

            {'p', u8"\u0CAB"},
            {'b', u8"\u0CAD"},

            {'n', u8"\u0CA3"},

            {'l', u8"\u0CB3"},

            {'s', u8"\u0CB6"},

            {'z', u8"\u0C99"},
        };

        // ============================================================
        // INDEPENDENT VOWELS
        // ============================================================

        constexpr Mapping independent_vowels[] = {
            {'a', u8"\u0C85"},
            {'i', u8"\u0C87"},
            {'u', u8"\u0C89"},
            {'e', u8"\u0C8E"},
            {'o', u8"\u0C92"},
        };

        // ============================================================
        // SHIFT + INDEPENDENT VOWELS
        // ============================================================

        constexpr Mapping shifted_independent_vowels[] = {
            {'a', u8"\u0C86"},
            {'i', u8"\u0C88"},
            {'u', u8"\u0C8A"},
            {'e', u8"\u0C8F"},
            {'o', u8"\u0C93"},

            {'r', u8"\u0C8B"},
            {'y', u8"\u0C90"},
            {'v', u8"\u0C94"},
        };

        // ============================================================
        // VOWEL SIGNS - NORMAL
        // ============================================================
        //
        // Used after a consonant.
        //
        // k + a = U+0C95U+0CBE
        // k + e = U+0C95U+0CC6
        // k + i = U+0C95U+0CBF
        // k + o = U+0C95U+0CCA
        // k + u = U+0C95U+0CC1
        //
        // ============================================================

        constexpr Mapping vowel_signs[] = {
            {'a', u8"\u0CBE"},
            {'e', u8"\u0CC6"},
            {'i', u8"\u0CBF"},
            {'o', u8"\u0CCA"},
            {'u', u8"\u0CC1"},
        };

        // ============================================================
        // VOWEL SIGNS - SHIFT
        // ============================================================
        //
        // k + Shift+a = U+0C95U+0CBE
        // k + Shift+e = U+0C95U+0CC7
        // k + Shift+i = U+0C95U+0CC0
        // k + Shift+o = U+0C95U+0CCB
        // k + Shift+u = U+0C95U+0CC2
        // k + Shift+y = U+0C95U+0CC8
        // k + Shift+v = U+0C95U+0CCC
        // k + Shift+r = U+0C95U+0CC3
        //
        // ============================================================

        constexpr Mapping shifted_vowel_signs[] = {
            {'a', u8"\u0CBE"},
            {'e', u8"\u0CC7"},
            {'i', u8"\u0CC0"},
            {'o', u8"\u0CCB"},
            {'u', u8"\u0CC2"},

            {'y', u8"\u0CC8"},
            {'v', u8"\u0CCC"},
            {'r', u8"\u0CC3"},
        };

        // ============================================================
        // SPECIAL SIGNS
        // ============================================================

        constexpr Mapping special_signs[] = {
            {'M', u8"\u0C82"}, // Anusvara
            {'h', u8"\u0C83"}, // Visarga
            {'x', u8"\u0CBC"},  // Nukta
        };

        // ============================================================
        // Lookup helper
        // ============================================================

        const char *lookup(
            const Mapping *mappings,
            std::size_t size,
            char key)
        {
            for (std::size_t index = 0; index < size; ++index)
            {
                if (mappings[index].key == key)
                {
                    return mappings[index].value;
                }
            }

            return nullptr;
        }

        // ============================================================
        // UTF-8 backspace helper
        // ============================================================

        void remove_last_utf8(std::string &text)
        {
            if (text.empty())
            {
                return;
            }

            std::size_t index = text.size() - 1;

            while (
                index > 0 &&
                (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80)
            {
                --index;
            }

            text.erase(index);
        }

        // ============================================================
        // Kannada digits
        // ============================================================

        const char *kannada_digit(char key)
        {
            constexpr const char *digits[] = {
                u8"\u0CE6",
                u8"\u0CE7",
                u8"\u0CE8",
                u8"\u0CE9",
                u8"\u0CEA",
                u8"\u0CEB",
                u8"\u0CEC",
                u8"\u0CED",
                u8"\u0CEE",
                u8"\u0CEF"};

            if (key < '0' || key > '9')
            {
                return nullptr;
            }

            return digits[key - '0'];
        }

        // ============================================================
        // Symbols used when Caps Lock is active
        // ============================================================

        std::string caps_symbol(char key, bool shifted)
        {
            // Shift + number row
            if (shifted && key >= '0' && key <= '9')
            {
                constexpr const char *symbols[] = {
                    u8"\uA8E0", // 0
                    u8"\uA8E1", // 1
                    u8"\uA8E2", // 2
                    u8"\uA8E3", // 3
                    u8"\uA8E4", // 4
                    u8"\uA8E5", // 5
                    u8"\uA8E6", // 6
                    u8"\uA8E7", // 7
                    u8"\uA8E8", // 8
                    u8"\uA8E9"  // 9
                };

                return symbols[key - '0'];
            }

            // Special bracket mappings
            if (shifted && (key == '[' || key == ']'))
            {
                return u8"\u0CE2";
            }

            if (shifted && key == '{')
            {
                return u8"\u0C8C";
            }

            if (shifted && key == '}')
            {
                return u8"\u0CE1";
            }

            constexpr Mapping symbols[] = {
                {'4', u8"\u20B9"},

                {'0', u8"\u0C81"},
                {'1', u8"\u0306"},
                {'2', u8"\u0304"},
                {',', u8"\u0327"},
                {'-', u8"\u0332"},
                {'3', u8"\u0347"},
                {'5', u8"\u030D"},
                {'6', u8"\u030E"},
                {'7', u8"\u0323"},
                {'\'', u8"\u0341"},
                {'8', u8"\u0307"},
                {'9', u8"\u0308"},
                {'.', u8"\u0324"},
                {'`', u8"\u02BB"},
            };

            const char *value = lookup(
                symbols,
                std::size(symbols),
                key);

            if (value != nullptr)
            {
                return value;
            }

            return std::string(
                1,
                shifted
                    ? static_cast<char>(
                          std::toupper(
                              static_cast<unsigned char>(key)))
                    : key);
        }

    } // namespace

    // ============================================================
    // Composer reset
    // ============================================================

    void Composer::reset()
    {
        text_.clear();

        consonant_pending_ = false;
        virama_pending_ = false;
        explicit_virama_ = false;
    }

    // ============================================================
    // Separator
    // ============================================================

    std::string Composer::separator()
    {
        // If a virama is waiting for another consonant,
        // terminate the sequence using ZWNJ.
        if (virama_pending_)
        {
            text_ += zwnj;
        }

        const std::string result = text_;

        reset();

        return result;
    }

    // ============================================================
    // Backspace
    // ============================================================

    bool Composer::backspace()
    {
        if (text_.empty())
        {
            return false;
        }

        // --------------------------------------------------------
        // Remove ZWJ + VIRAMA
        // --------------------------------------------------------

        const std::string zwjVirama(zwj_virama);

        if (
            text_.size() >= zwjVirama.size() &&
            text_.compare(
                text_.size() - zwjVirama.size(),
                zwjVirama.size(),
                zwjVirama) == 0)
        {
            text_.erase(
                text_.size() - zwjVirama.size());

            consonant_pending_ = true;
            virama_pending_ = true;
            explicit_virama_ = true;

            return true;
        }

        // --------------------------------------------------------
        // Remove ZWNJ
        // --------------------------------------------------------

        const std::string zwnjString(zwnj);

        if (
            text_.size() >= zwnjString.size() &&
            text_.compare(
                text_.size() - zwnjString.size(),
                zwnjString.size(),
                zwnjString) == 0)
        {
            text_.erase(
                text_.size() - zwnjString.size());

            consonant_pending_ = false;
            virama_pending_ = true;
            explicit_virama_ = true;

            return true;
        }

        // --------------------------------------------------------
        // Remove last UTF-8 code point
        // --------------------------------------------------------

        remove_last_utf8(text_);

        consonant_pending_ = false;

        const std::string viramaString(virama);

        virama_pending_ =
            text_.size() >= viramaString.size() &&
            text_.compare(
                text_.size() - viramaString.size(),
                viramaString.size(),
                viramaString) == 0;
        explicit_virama_ = virama_pending_;

        return true;
    }

    // ============================================================
    // Feed key
    // ============================================================

    std::string Composer::feed(
        char key,
        bool shifted,
        bool caps_lock,
        bool scroll_lock,
        bool num_lock,
        bool alt)
    {
        // ========================================================
        // Scroll Lock + number
        // ========================================================

        if (
            scroll_lock &&
            !caps_lock &&
            std::isdigit(
                static_cast<unsigned char>(key)))
        {
            const std::string result =
                text_ + std::string(1, key);

            reset();

            return result;
        }

        // ========================================================
        // Num Lock + number
        // ========================================================

        if (
            num_lock &&
            !caps_lock &&
            std::isdigit(
                static_cast<unsigned char>(key)))
        {
            const char *digit = kannada_digit(key);

            if (digit == nullptr)
            {
                const std::string result =
                    text_ + std::string(1, key);

                reset();

                return result;
            }

            std::string result = text_;

            if (virama_pending_)
            {
                result += zwnj;
            }

            result += digit;
            reset();

            return result;
        }

        // ========================================================
        // Num Lock special characters
        // ========================================================

        if (num_lock && !caps_lock && key == '`')
        {
            const std::string result =
                text_ + (shifted ? "~" : u8"\u02BB");

            reset();

            return result;
        }

        if (num_lock && !caps_lock && key == '\'')
        {
            const std::string result =
                text_ + (apostrophe_toggle_ ? u8"\u2018" : u8"\u2019");

            apostrophe_toggle_ = !apostrophe_toggle_;

            reset();

            return result;
        }

        if (num_lock && !caps_lock && key == ',')
        {
            const std::string result =
                text_ + ",";

            reset();

            return result;
        }

        // ========================================================
        // Standalone special signs
        //
        // Shift+h -> U+0C83
        // Shift+m -> U+0C82
        // Shift+x -> U+0CBC
        //
        // Only when we are NOT composing a consonant.
        // ========================================================

        if (
            shifted &&
            !caps_lock &&
            !consonant_pending_ &&
            !virama_pending_)
        {
            if (key == 'h')
            {
                const std::string result = text_ + u8"\u0C83";
                reset();
                return result;
            }

            if (key == 'm')
            {
                const std::string result = text_ + u8"\u0C82";
                reset();
                return result;
            }

            if (key == 'x')
            {
                const std::string result = text_ + u8"\u0CBC";
                reset();
                return result;
            }

            if (key == '[' || key == ']')
            {
                const std::string result = text_ + u8"\u0CE2";
                reset();
                return result;
            }
        }

        // ========================================================
        // Caps Lock layer
        // ========================================================

        if (caps_lock)
        {

            // Alt + Shift + A
            if (
                alt &&
                shifted &&
                key == 'a')
            {
                std::string result = text_;

                if (virama_pending_)
                {
                    result += zwnj;
                }

                result += u8"\u0CF1";

                reset();

                return result;
            }

            // Alt + Shift + Z
            if (
                alt &&
                shifted &&
                key == 'z')
            {
                std::string result = text_;

                if (virama_pending_)
                {
                    result += zwnj;
                }

                result += u8"\u0CF2";

                reset();

                return result;
            }

            const std::string symbol =
                caps_symbol(key, shifted);

            std::string result = text_;

            if (virama_pending_)
            {
                result += zwnj;
            }

            result += symbol;

            reset();

            return result;
        }

        // ========================================================
        // Consonant lookup
        // ========================================================

        const char *consonant =
            shifted
                ? lookup(
                      shifted_consonants,
                      std::size(shifted_consonants),
                      key)
                : lookup(
                      consonants,
                      std::size(consonants),
                      key);

        if (consonant != nullptr)
        {

            // ----------------------------------------------------
            // Existing virama means an ottakshara/conjunct.
            //
            // Example:
            //
            // k + f + k
            //
            // previous:
            // U+0C95 U+0CCD
            //
            // new consonant:
            // U+0C95
            //
            // resulting sequence:
            // U+0C95 U+0CCD + ZWJ + U+0C95
            //
            // ----------------------------------------------------

            if (virama_pending_)
            {
                const bool was_explicit_virama = explicit_virama_;
                text_ += consonant;
                consonant_pending_ = true;
                virama_pending_ = false;
                explicit_virama_ = false;
                if (!was_explicit_virama && default_vowel_ == DefaultVowel::Virama)
                {
                    text_ += virama;
                }
                return {};
            }

            if (default_vowel_ == DefaultVowel::Virama)
            {
                if (text_.size() >= std::string(virama).size() &&
                    text_.compare(text_.size() - std::string(virama).size(),
                                  std::string(virama).size(), virama) == 0)
                {
                    text_.erase(text_.size() - std::string(virama).size());
                }
                text_ += consonant;
                text_ += virama;
                consonant_pending_ = true;
                virama_pending_ = false;
                return {};
            }

            text_ += consonant;

            consonant_pending_ = true;
            virama_pending_ = false;

            return {};
        }

        // ========================================================
        // VIRAMA
        //
        // k + f -> U+0C95 U+0CCD
        // ========================================================

        if (
            key == 'f' &&
            consonant_pending_)
        {
            if (shifted)
            {
                text_ += zwj_virama;

                consonant_pending_ = false;
                virama_pending_ = true;
                explicit_virama_ = true;

                return {};
            }

            if (default_vowel_ == DefaultVowel::Virama &&
                !virama_pending_ &&
                text_.size() >= std::string(virama).size() &&
                text_.compare(text_.size() - std::string(virama).size(),
                              std::string(virama).size(), virama) == 0)
            {
                virama_pending_ = true;
                explicit_virama_ = true;
                return {};
            }

            text_ += virama;

            consonant_pending_ = false;
            virama_pending_ = true;
            explicit_virama_ = true;

            return {};
        }

        if (default_vowel_ == DefaultVowel::Virama && key == 'f')
        {
            text_ += virama;
            virama_pending_ = true;
            explicit_virama_ = true;
            return {};
        }

        // ========================================================
        // Repeated VIRAMA
        //
        // k + f + f
        //
        // terminate using ZWNJ
        // ========================================================

        if (
            key == 'f' &&
            virama_pending_)
        {
            text_ += zwnj;

            const std::string result = text_;

            reset();

            return result;
        }

        // f is a composition control key in Nudi.  When there is no
        // consonant to which it can attach, consume it rather than emitting
        // a Latin f.
        if (key == 'f')
        {
            const std::string result = text_;
            reset();
            return result;
        }

        // ========================================================
        // Consonant + vowel/sign/special
        // ========================================================

        if (consonant_pending_)
        {

            // ----------------------------------------------------
            // Shift + h -> visarga
            //
            // U+0C95 + Shift+h -> U+0C95U+0C83
            // ----------------------------------------------------

            if (
                shifted &&
                key == 'h')
            {
                text_ += u8"\u0C83";

                const std::string result = text_;

                reset();

                return result;
            }

            // ----------------------------------------------------
            // Shift + m -> anusvara
            //
            // U+0C95 + Shift+m -> U+0C95U+0C82
            // ----------------------------------------------------

            if (
                shifted &&
                key == 'm')
            {
                text_ += u8"\u0C82";

                consonant_pending_ = false;
                virama_pending_ = false;
                explicit_virama_ = false;

                return {};
            }

            // ----------------------------------------------------
            // Shift + x -> nukta
            //
            // U+0C95 + Shift+x -> U+0C95U+0CBC
            // ----------------------------------------------------

            if (
                shifted &&
                key == 'x')
            {
                text_ += u8"\u0CBC";

                const std::string result = text_;

                reset();

                return result;
            }

            // ----------------------------------------------------
            // Vowel sign
            // ----------------------------------------------------

            const char *sign =
                shifted
                    ? lookup(
                          shifted_vowel_signs,
                          std::size(shifted_vowel_signs),
                          key)
                    : lookup(
                          vowel_signs,
                          std::size(vowel_signs),
                          key);

            if (sign != nullptr)
            {
                if (default_vowel_ == DefaultVowel::Virama && key == 'a')
                {
                    text_.erase(text_.size() - std::string(virama).size());
                    consonant_pending_ = false;
                    virama_pending_ = false;
                    explicit_virama_ = false;
                    return {};
                }
                if (default_vowel_ == DefaultVowel::Virama &&
                    text_.size() >= std::string(virama).size() &&
                    text_.compare(text_.size() - std::string(virama).size(),
                                  std::string(virama).size(), virama) == 0)
                {
                    text_.erase(text_.size() - std::string(virama).size());
                }
                text_ += sign;

                consonant_pending_ = false;
                virama_pending_ = false;
                explicit_virama_ = false;

                return {};
            }

            if (default_vowel_ == DefaultVowel::Virama &&
                key == 'a' &&
                text_.size() >= std::string(virama).size() &&
                text_.compare(text_.size() - std::string(virama).size(),
                              std::string(virama).size(), virama) == 0)
            {
                text_.erase(text_.size() - std::string(virama).size());
                consonant_pending_ = false;
                virama_pending_ = false;
                explicit_virama_ = false;
                return {};
            }
        }

        // ========================================================
        // Independent vowel
        // ========================================================

        const char *vowel =
            shifted
                ? lookup(
                      shifted_independent_vowels,
                      std::size(shifted_independent_vowels),
                      key)
                : lookup(
                      independent_vowels,
                      std::size(independent_vowels),
                      key);

        // ========================================================
        // Unknown key / normal character
        // ========================================================

        std::string result =
            text_ +
            (vowel != nullptr
                 ? vowel
                 : std::string(1, key));

        reset();

        return result;
    }

} // namespace nudi
