// Project identifier: 0E04A31E0D60C01986ACB20081C9D8722A2619B6

// The important thing to know is how to declare a P2random::GenStream and
// call its init() member function. You do not need to read any further.

#pragma once

#include <charconv>
#include <cstddef>
#include <cinttypes>
#include <istream>
#include <streambuf>

class P2random {
    // Adapted from http://www.pcg-random.org
    struct Prng {
        using result_type = uint32_t;
        explicit Prng() noexcept = default;
        explicit Prng(uint64_t seq) noexcept { reseed(seq); }
        auto reseed(uint64_t seq) noexcept -> void { reseed(init_state, seq); }

        auto reseed(uint64_t state, uint64_t seq) noexcept -> void {
            state_ = 0U;
            inc_ = (seq << 1U) | 1U;
            operator()();
            state_ += state;
            operator()();
        } // reseed()

        auto operator()() noexcept -> result_type {
            auto const oldstate = state_;
            state_ = oldstate * 6364136223846793005ULL + (inc_ | 1);
            auto const xorshifted =
                static_cast<result_type>(((oldstate >> 18U) ^ oldstate) >> 27U);
            auto const rot = static_cast<result_type>(oldstate >> 59U);
            return (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31));
        } // operator()()

    private:
        constexpr static auto init_state = 0x853c49e6748fea9bULL;
        constexpr static auto init_seq = 0xda3e39cb94b95bdbULL;

        uint64_t state_ = init_state;
        uint64_t inc_ = init_seq;
    }; // Prng{}

    // Generates one order at a time into a single fixed line buffer. Nothing
    // here allocates, so the footprint is the same for 10 orders as for
    // 10 million.
    class GenBuf : public std::streambuf {
        constexpr static uint32_t max_price = 100;
        constexpr static uint32_t max_quantity = 50;

        // Longest possible line, term by term: 11 for int(timestamp)
        // including a sign, 1 space, 4 for "SELL", 2 for " T" and up to 10
        // digits of trader, 2 for " S" and up to 10 of stock, 2 for " $"
        // and up to 3 of price, 2 for " #" and up to 2 of quantity, and
        // 1 newline.
        constexpr static std::size_t line_max = 64;
        static_assert(line_max >= 11 + 1 + 4 + 2 + 10 + 2 + 10 + 2 + 3 + 2 + 2 + 1,
                      "line buffer must hold the longest possible order");

        // The whole of this class's memory footprint, for any input size.
        char line[line_max];
        Prng rng;
        uint32_t num_traders = 0;
        uint32_t num_stocks = 0;
        uint32_t num_orders = 0;
        uint32_t arrival_rate = 1;
        long double timestamp = 0;

        // to_chars formats integers without allocating and without a locale
        // lookup, which is what keeps generation cheap enough to do inline
        // with reading.
        static char *put_int(char *out, int value) {
            return std::to_chars(out, out + 11, value).ptr;
        } // put_int()

        static char *put_uint(char *out, uint32_t value) {
            return std::to_chars(out, out + 10, value).ptr;
        } // put_uint()

        static char *put_text(char *out, const char *text) {
            while (*text) {
                *out++ = *text++;
            } // while ..text
            return out;
        } // put_text()

        // Every rng() call below, and their order, must match the order the
        // previous stringstream implementation used. Reordering these lines
        // silently desynchronizes every existing test file.
        char *put_order(char *out) {
            uint32_t time_increase = rng() % arrival_rate + 1;
            timestamp += 1.0l / time_increase;
            const char *buy_or_sell = rng() % 2 == 0 ? "BUY" : "SELL";
            uint32_t trader_num = rng() % num_traders;
            uint32_t stock_num = rng() % num_stocks;
            uint32_t price = rng() % max_price + 1;
            uint32_t quantity = rng() % max_quantity + 1;

            out = put_int(out, int(timestamp));
            *out++ = ' ';
            out = put_text(out, buy_or_sell);
            out = put_text(out, " T");
            out = put_uint(out, trader_num);
            out = put_text(out, " S");
            out = put_uint(out, stock_num);
            out = put_text(out, " $");
            out = put_uint(out, price);
            out = put_text(out, " #");
            out = put_uint(out, quantity);
            *out++ = '\n';
            return out;
        } // put_order()

    protected:
        int_type underflow() override {
            if (gptr() < egptr()) {
                return traits_type::to_int_type(*gptr());
            } // if ..gptr

            // Checked before decrementing, so the counter never wraps and
            // an exhausted stream stays exhausted.
            if (num_orders == 0) {
                return traits_type::eof();
            } // if ..num_orders
            --num_orders;

            setg(line, line, put_order(line));
            return traits_type::to_int_type(*gptr());
        } // underflow()

    public:
        GenBuf() { setg(line, line, line); }

        void init(uint32_t seed, uint32_t traders, uint32_t stocks,
                  uint32_t orders, uint32_t rate) {
            rng.reseed(seed);
            num_traders = traders;
            num_stocks = stocks;
            num_orders = orders;
            arrival_rate = rate;
            timestamp = 0;
            setg(line, line, line);
        } // init()
    }; // GenBuf{}

    // GenBuf must be fully constructed before std::istream is handed its
    // address, and base classes initialize in declaration order. Holding it
    // in a base rather than a member is what makes that ordering legal
    // instead of merely lucky.
    struct GenBufHolder {
        GenBuf buf;
    }; // GenBufHolder{}

public:
    // An istream that manufactures PR orders as they are read. Declare one,
    // call init() only in PR mode, and read from it exactly like cin. Before
    // init() it is simply an empty stream.
    class GenStream : private GenBufHolder, public std::istream {
    public:
        GenStream() : GenBufHolder{}, std::istream{&buf} {}

        void init(uint32_t seed, uint32_t num_traders, uint32_t num_stocks,
                  uint32_t num_orders, uint32_t arrival_rate) {
            buf.init(seed, num_traders, num_stocks, num_orders, arrival_rate);
            clear();
        } // init()
    }; // GenStream{}
}; // P2random
