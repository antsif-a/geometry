export module logger;

import std;
using std::string;
using std::string_view;
using std::format_string;
using std::make_format_args;
using std::vformat;
using std::format;
using std::forward;
using std::cout;
using std::endl;
using std::chrono::utc_clock;

namespace logger {
    namespace detail {
        template<unsigned... digits>
        struct to_chars { static const char value[]; };

        template<unsigned... digits>
        constexpr char to_chars<digits...>::value[] = {('0' + digits)..., 0};

        template<unsigned rem, unsigned... digits>
        struct explode : explode<rem / 10, rem % 10, digits...> {};

        template<unsigned... digits>
        struct explode<0, digits...> : to_chars<digits...> {};

        template<>
        struct explode<0> : to_chars<0> {};
    }
}

export namespace logger {
    enum Level {
        Error, Warn, Notice, Info, Debug
    };

    template<unsigned n>
    struct to_str : detail::explode<n> {};

    enum Style {
        normal = 0,
        bold, dim, italic, underline,
        slow_blink, rapid_blink,
        invert, hide, strike,

        normal_intensity = 22,
        normal_fg = 39,
        normal_bg = 49,
        normal_underline = 59,
        superscript = 73,
        subscript = 74,
        normal_script = 75
    };

    namespace fg {
        enum { black = 30, red, green, yellow, blue, magenta, cyan, white };

        namespace bright {
            enum { black = 90, red, green, yellow, blue, magenta, cyan, white };
        };
    };

    namespace bg {
        enum { black = 40, red, green, yellow, blue, magenta, cyan, white };

        namespace bright {
            enum { black = 100, red, green, yellow, blue, magenta, cyan, white };
        };
    };

    template<int level>
    constexpr unsigned level_to_color() {
        switch (level) {
            case Error:
                return fg::red;
            case Warn:
                return fg::bright::yellow;
            case Notice:
                return bold;
            case Info:
                return normal;
            case Debug:
                return dim;
        };
        return normal;
    }

    template<unsigned... Codes>
    constexpr auto escape() {
        auto _ = [] () {
            string result = "\x1b[";
            if constexpr (sizeof...(Codes) > 0) {
                ((result += to_str<Codes>::value, result += ";"), ...);
                result.pop_back();
            }
            result += 'm';
            return result;
        };
        return _();
    }

    template<int level, typename... Args>
    void log(format_string<Args...> fmt, Args&&... args) {
        constexpr auto color = level_to_color<level>();
        cout << format("{0:%x} {0:%X}", utc_clock::now());
        cout << escape<normal>();
        cout << ": ";
        cout << escape<color, bold>();
        cout << vformat(fmt.get(), make_format_args(args...));
        cout << escape<normal>();
        cout << endl;
    }

    template<typename... Args>
    void error(format_string<Args...> fmt, Args&&... args) {
        log<Error>(fmt, forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(format_string<Args...> fmt, Args&&... args) {
        log<Warn>(fmt, forward<Args>(args)...);
    }

    template<typename... Args>
    void notice(format_string<Args...> fmt, Args&&... args) {
        log<Notice>(fmt, forward<Args>(args)...);
    }

    template<typename... Args>
    void info(format_string<Args...> fmt, Args&&... args) {
        log<Info>(fmt, forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(format_string<Args...> fmt, Args&&... args) {
        log<Debug>(fmt, forward<Args>(args)...);
    }
};

