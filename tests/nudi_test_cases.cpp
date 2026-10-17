#include "nudi_test_cases.hpp"

namespace nudi::tests
{

// ============================================================
// Reference / word tests
// ============================================================

const std::vector<ReferenceTestCase> &reference_test_cases()
{
    static const std::vector<ReferenceTestCase> tests =
    {
        {
            "WORD-001",
            "Karnataka",
            "krfnAqk",
            "ಕರ್ನಾಟಕ"
        },

        {
            "WORD-002",
            "kanaka",
            "knk",
            "ಕನಕ"
        },

        {
            "WORD-003",
            "rF conjunct",
            "rFk",
            "ರ‍್ಕ"
        },

        {
            "WORD-004",
            "ordinary r conjunct",
            "rfk",
            "ರ್ಕ"
        },

        {
            "WORD-005",
            "rF conjunct after vowel",
            "arFk",
            "ಅರ‍್ಕ"
        },

        {
            "WORD-006",
            "ordinary r conjunct after vowel",
            "arfk",
            "ಅರ್ಕ"
        },

        {
            "WORD-007",
            "mantri",
            "mMtfri",
            "ಮಂತ್ರಿ"
        },

        {
            "WORD-008",
            "shifted consonant and vowel",
            "KkE",
            "ಖಕೇ"
        },

        {
            "WORD-009",
            "independent vowels with signs",
            "aMiH",
            "ಅಂಇಃ"
        },

        {
            "WORD-010",
            "nukta after consonant",
            "jX",
            "ಜ಼"
        },

  

    };

    return tests;
}


// ============================================================
// Single key tests
// ============================================================

const std::vector<KeyTestCase> &key_test_cases()
{
    static const std::vector<KeyTestCase> tests =
    {
        // ----------------------------------------------------
        // Shift mappings
        // ----------------------------------------------------

        {
            "KEY-001",
            "Shift consonant",
            'q',
            true,
            false,
            false,
            false,
            false,
            false,
            "ಠ"
        },

        {
            "KEY-002",
            "Shift independent vowel",
            'a',
            true,
            false,
            false,
            false,
            false,
            false,
            "ಆ"
        },

        {
            "KEY-003",
            "Normal digit without Num Lock",
            '5',
            false,
            false,
            false,
            false,
            false,
            false,
            "5"
        },

        {
            "KEY-004",
            "Kannada digit with Num Lock",
            '5',
            false,
            false,
            false,
            false,
            false,
            true,
            "೫"
        },

        {
            "KEY-005",
            "ASCII digit with Scroll Lock",
            '5',
            false,
            false,
            false,
            false,
            true,
            false,
            "5"
        },

        {
            "KEY-006",
            "Caps Lock symbol",
            '4',
            false,
            true,
            false,
            false,
            false,
            false,
            "₹"
        },

        {
            "KEY-009",
            "Caps Lock zero mapping",
            '0',
            false,
            true,
            false,
            false,
            false,
            false,
            "ಁ"
        },

        {
            "KEY-007",
            "Caps Lock shifted closing bracket",
            '}',
            true,
            true,
            false,
            false,
            false,
            false,
            "ೡ"
        },

        {
            "KEY-008",
            "Caps Lock shifted H remains Latin",
            'h',
            true,
            true,
            false,
            false,
            false,
            false,
            "H"
        },

 
        // ----------------------------------------------------
        // Caps + Alt + Shift
        // ----------------------------------------------------

        {
            "CAPS-001",
            "Caps + Alt + Shift + A",
            'a',
            true,
            true,
            true,
            false,
            false,
            false,
            "ೱ"
        },

        {
            "CAPS-002",
            "Caps + Alt + Shift + Z",
            'z',
            true,
            true,
            true,
            false,
            false,
            false,
            "ೲ"
        }
    };

    return tests;
}


// ============================================================
// Composition tests
// ============================================================

const std::vector<CompositionTestCase> &composition_test_cases()
{
    static const std::vector<CompositionTestCase> tests =
    {
        {
            "COMP-001",
            "Consonant + vowel",
            "ka",
            "ಕಾ"
        },


        {
            "COMP-005",
            "Repeated virama",
            "kf",
            "ಕ್"
        }
    };

    return tests;
}

} // namespace nudi::tests
