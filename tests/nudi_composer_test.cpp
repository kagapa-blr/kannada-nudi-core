#include "nudi_composer.hpp"
#include "nudi_test_cases.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

// ============================================================
// Test statistics
// ============================================================

struct TestStats
{
    int total = 0;
    int passed = 0;
    int failed = 0;
};


// ============================================================
// Test reporter
// ============================================================

class TestReporter
{
public:

    explicit TestReporter(const std::string &path)
        : file_(path)
    {
        if (!file_.is_open())
        {
            std::cerr
                << "ERROR: Could not create test report: "
                << path
                << '\n';

            valid_ = false;
            return;
        }

        file_
            << "========================================\n"
            << "       NUDI COMPOSER TEST RESULTS\n"
            << "========================================\n";
    }


    bool valid() const
    {
        return valid_;
    }


    // ========================================================
    // Record test result
    //
    // Detailed result -> file
    // Nothing printed for individual tests -> console
    // ========================================================

    bool check(
        TestStats &stats,
        const std::string &id,
        const std::string &name,
        const std::string &input,
        const std::string &expected,
        const std::string &actual)
    {
        ++stats.total;

        const bool passed =
            expected == actual;

        if (passed)
            ++stats.passed;
        else
            ++stats.failed;


        // ----------------------------------------------------
        // Detailed result -> file only
        // ----------------------------------------------------

        file_
            << "\n========================================\n"
            << "ID:       " << id << '\n'
            << "Test:     " << name << '\n'
            << "Input:    " << input << '\n'
            << "Expected: " << expected << '\n'
            << "Actual:   " << actual << '\n'
            << "Result:   " << (passed ? "PASS" : "FAIL") << '\n'
            << "========================================\n";


        return passed;
    }


    // ========================================================
    // Final summary
    //
    // Summary -> file + console
    // ========================================================

    void summary(const TestStats &stats)
    {
        // ----------------------------------------------------
        // Summary -> file
        // ----------------------------------------------------

        file_
            << "\n\n========================================\n"
            << "           TEST SUMMARY\n"
            << "========================================\n"
            << "Total :  " << stats.total << '\n'
            << "Passed:  " << stats.passed << '\n'
            << "Failed:  " << stats.failed << '\n'
            << "========================================\n";

        file_
            << (stats.failed == 0
                    ? "ALL TESTS PASSED\n"
                    : "TESTS FAILED\n");


        // ----------------------------------------------------
        // Summary -> console
        // ----------------------------------------------------

        std::cout
            << "\n========================================\n"
            << "           TEST SUMMARY\n"
            << "========================================\n"
            << "Total :  " << stats.total << '\n'
            << "Passed:  " << stats.passed << '\n'
            << "Failed:  " << stats.failed << '\n'
            << "========================================\n";


        if (stats.failed == 0)
        {
            std::cout
                << "ALL TESTS PASSED\n"
                << "========================================\n";
        }
        else
        {
            std::cout
                << "TESTS FAILED\n"
                << "See detailed results:\n"
                << "nudi-test-results.txt\n"
                << "========================================\n";
        }
    }


private:

    std::ofstream file_;
    bool valid_ = true;
};


// ============================================================
// Feed Nudi reference notation
//
// Uppercase ASCII = Shift + key
// ============================================================

std::string feed_reference(
    nudi::Composer &composer,
    const std::string &input)
{
    std::string committed;

    for (char key : input)
    {
        const bool shifted =
            key >= 'A' &&
            key <= 'Z';

        committed += composer.feed(
            shifted
                ? static_cast<char>(key - 'A' + 'a')
                : key,
            shifted);
    }

    return committed + composer.preedit();
}


// ============================================================
// Run reference tests
// ============================================================

void run_reference_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    const auto &tests =
        nudi::tests::reference_test_cases();

    for (const auto &test : tests)
    {
        composer.reset();

        const std::string actual =
            feed_reference(
            composer,
            test.input);

        reporter.check(
            stats,
            test.id,
            test.name,
            test.input,
            test.expected,
            actual);
    }
}


// ============================================================
// Run key tests
// ============================================================

void run_key_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    const auto &tests =
        nudi::tests::key_test_cases();

    for (const auto &test : tests)
    {
        composer.reset();

        const std::string actual =
            composer.feed(
                test.key,
                test.shift,
                test.caps_lock,
                test.ctrl,
                test.num_lock,
                test.alt) +
            composer.preedit();


        std::string input;

        if (test.shift)
            input += "Shift+";

        if (test.alt)
            input += "Alt+";

        if (test.ctrl)
            input += "Ctrl+";

        if (test.scroll_lock)
            input += "ScrollLock+";

        if (test.num_lock)
            input += "NumLock+";

        input += test.key;


        reporter.check(
            stats,
            test.id,
            test.name,
            input,
            test.expected,
            actual);
    }
}


// ============================================================
// Run composition tests
// ============================================================

void run_composition_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    const auto &tests =
        nudi::tests::composition_test_cases();

    for (const auto &test : tests)
    {
        composer.reset();

        feed_reference(
            composer,
            test.input);

        reporter.check(
            stats,
            test.id,
            test.name,
            test.input,
            test.expected,
            composer.preedit());
    }
}


// ============================================================
// Basic consonant tests
// ============================================================

void run_consonant_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    struct ConsonantTest
    {
        const char *id;
        const char *name;
        char key;
        const char *expected;
    };

    const ConsonantTest tests[] =
    {
        {"CON-001", "Ra",  'r', "ರ"},
        {"CON-002", "Sa",  's', "ಸ"},
        {"CON-003", "Da",  'd', "ದ"},
        {"CON-004", "Dda", 'w', "ಡ"},
        {"CON-005", "Ta",  'q', "ಟ"},
        {"CON-006", "Ta2", 't', "ತ"},
        {"CON-007", "Ya",  'y', "ಯ"},
        {"CON-008", "Pa",  'p', "ಪ"},
        {"CON-009", "Ga",  'g', "ಗ"},
        {"CON-010", "Ha",  'h', "ಹ"},
        {"CON-011", "Ja",  'j', "ಜ"},
        {"CON-012", "La",  'l', "ಲ"},
        {"CON-013", "Ka",  'k', "ಕ"},
        {"CON-014", "Nya", 'z', "ಞ"},
        {"CON-015", "Sha", 'x', "ಷ"},
        {"CON-016", "Cha", 'c', "ಚ"},
        {"CON-017", "Va",  'v', "ವ"},
        {"CON-018", "Ba",  'b', "ಬ"},
        {"CON-019", "Na",  'n', "ನ"},
        {"CON-020", "Ma",  'm', "ಮ"}
    };


    for (const auto &test : tests)
    {
        composer.reset();

        composer.feed(test.key);

        reporter.check(
            stats,
            test.id,
            test.name,
            std::string(1, test.key),
            test.expected,
            composer.preedit());
    }
}


// ============================================================
// Default vowel tests
// ============================================================

void run_default_vowel_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    composer.set_default_vowel(
        nudi::Composer::DefaultVowel::Virama);

    composer.reset();

    const std::string input =
        "kanaka";

    const std::string actual =
        feed_reference(
        composer,
        input);

    reporter.check(
        stats,
        "VOWEL-001",
        "Default vowel = Virama",
        input,
        "ಕನಕ",
        actual);


    composer.set_default_vowel(
        nudi::Composer::DefaultVowel::A);
}


// ============================================================
// Backspace test
// ============================================================

void run_backspace_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    composer.reset();

    composer.feed('k');

    composer.backspace();

    reporter.check(
        stats,
        "STATE-001",
        "Backspace",
        "k + Backspace",
        "empty",
        composer.empty()
            ? "empty"
            : composer.preedit());
}


// ============================================================
// Lock-layer state tests
// ============================================================

void run_lock_state_tests(
    nudi::Composer &composer,
    TestReporter &reporter,
    TestStats &stats)
{
    nudi::Composer quote_composer;

    const std::string first_quote =
        quote_composer.feed('\'', false, false, false, true, false);
    const std::string second_quote =
        quote_composer.feed('\'', false, false, false, true, false);
    const std::string quotes = first_quote + second_quote;

    reporter.check(
        stats,
        "STATE-002",
        "NumLock apostrophe alternation",
        "NumLock+' + NumLock+'",
        "’‘",
        quotes);

    composer.reset();
    const std::string a_result = composer.feed('a');
    const std::string f_result = composer.feed('f');

    reporter.check(
        stats,
        "STATE-003",
        "f after independent vowel",
        "af",
        "ಅ",
        a_result + f_result + composer.preedit());
}

} // namespace


// ============================================================
// Main
// ============================================================

int main()
{
    // --------------------------------------------------------
    // Determine test output directory
    // --------------------------------------------------------

    const std::filesystem::path output_dir =
        NUDI_TEST_OUTPUT_DIR;


    // --------------------------------------------------------
    // Create output directory if it does not exist
    // --------------------------------------------------------

    try
    {
        std::filesystem::create_directories(
            output_dir);
    }
    catch (const std::filesystem::filesystem_error &error)
    {
        std::cerr
            << "ERROR: Could not create test output directory:\n"
            << output_dir
            << "\n"
            << error.what()
            << '\n';

        return 1;
    }


    // --------------------------------------------------------
    // Test report path
    // --------------------------------------------------------

    const std::filesystem::path report_file =
        output_dir / "nudi-test-results.txt";


    // --------------------------------------------------------
    // Create reporter
    // --------------------------------------------------------

    TestReporter reporter(
        report_file.string());


    if (!reporter.valid())
        return 1;


    // --------------------------------------------------------
    // Create composer
    // --------------------------------------------------------

    nudi::Composer composer;

    TestStats stats;


    // --------------------------------------------------------
    // Console header
    // --------------------------------------------------------

    std::cout
        << "Running Nudi Composer Tests...\n"
        << "Detailed results: "
        << report_file
        << "\n";


    // --------------------------------------------------------
    // Run tests
    // --------------------------------------------------------

    run_consonant_tests(
        composer,
        reporter,
        stats);

    run_reference_tests(
        composer,
        reporter,
        stats);

    run_key_tests(
        composer,
        reporter,
        stats);

    run_composition_tests(
        composer,
        reporter,
        stats);

    run_default_vowel_tests(
        composer,
        reporter,
        stats);

    run_backspace_tests(
        composer,
        reporter,
        stats);

    run_lock_state_tests(
        composer,
        reporter,
        stats);


    // --------------------------------------------------------
    // Final summary
    // --------------------------------------------------------

    reporter.summary(stats);


    // --------------------------------------------------------
    // Exit status
    //
    // 0 = all tests passed
    // 1 = one or more tests failed
    // --------------------------------------------------------

    return stats.failed == 0
        ? 0
        : 1;
}
