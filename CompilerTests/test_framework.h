#pragma once
// A tiny, dependency-free unit-test framework that runs on Windows, Linux and macOS.
//
// Usage:
//   TEST(MyTest) {
//       CHECK(1 + 1 == 2);
//       CHECK_EQ(std::string("a"), std::string("a"));
//   }
// and call microtest::run() from main().

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace microtest {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back(TestCase{ name, fn });
    }
};

// Thrown when a CHECK fails; carries a human-readable description.
struct AssertionFailure {
    std::string message;
};

inline void fail(const std::string& file, int line, const std::string& expression) {
    std::ostringstream os;
    os << file << ":" << line << ": CHECK failed: " << expression;
    throw AssertionFailure{ os.str() };
}

// Returns true if calling `fn` throws anything. Standard output and error are
// redirected to a sink while the call runs, so expected diagnostics stay quiet.
template <typename F>
bool throws(F fn) {
    std::ostringstream sink;
    std::streambuf* oldOut = std::cout.rdbuf(sink.rdbuf());
    std::streambuf* oldErr = std::cerr.rdbuf(sink.rdbuf());

    bool threw = false;
    try {
        fn();
    }
    catch (...) {
        threw = true;
    }

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);
    return threw;
}

inline int run() {
    int failed = 0;
    for (const auto& test : registry()) {
        try {
            test.fn();
            std::cout << "[ OK ] " << test.name << "\n";
        }
        catch (const AssertionFailure& e) {
            std::cout << "[FAIL] " << test.name << "\n  " << e.message << "\n";
            ++failed;
        }
        catch (const std::exception& e) {
            std::cout << "[FAIL] " << test.name << "\n  unexpected exception: " << e.what() << "\n";
            ++failed;
        }
        catch (...) {
            std::cout << "[FAIL] " << test.name << "\n  unexpected unknown exception\n";
            ++failed;
        }
    }

    std::cout << "\n" << registry().size() << " test(s) run, "
        << failed << " failed.\n";
    return failed == 0 ? 0 : 1;
}

} // namespace microtest

#define TEST(name)                                                       \
    static void name();                                                  \
    static ::microtest::Registrar microtest_reg_##name(#name, name);     \
    static void name()

#define CHECK(expression)                                                \
    do {                                                                 \
        if (!(expression))                                               \
            ::microtest::fail(__FILE__, __LINE__, #expression);          \
    } while (0)

#define CHECK_EQ(actual, expected)                                       \
    do {                                                                 \
        if (!((actual) == (expected)))                                   \
            ::microtest::fail(__FILE__, __LINE__,                        \
                std::string(#actual) + " == " + #expected);              \
    } while (0)

#define CHECK_THROWS(expression)                                         \
    do {                                                                 \
        if (!::microtest::throws([&]() { (void)(expression); }))         \
            ::microtest::fail(__FILE__, __LINE__,                        \
                std::string("expected exception: ") + #expression);      \
    } while (0)
