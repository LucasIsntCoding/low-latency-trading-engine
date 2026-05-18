#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "llt/book/order_book.hpp"
#include "llt/core/config.hpp"
#include "llt/core/metrics.hpp"
#include "llt/core/spsc_ring.hpp"
#include "llt/core/time.hpp"
#include "llt/feed/simulated_feed.hpp"
#include "llt/persistence/csv_writer.hpp"
#include "llt/persistence/redis_client.hpp"
#include "llt/portfolio/portfolio.hpp"
#include "llt/strategy/mean_reversion_strategy.hpp"
#include "llt/trading/order_manager.hpp"
#include "llt/trading/risk_manager.hpp"

namespace fs = std::filesystem;
using namespace llt;

namespace {
std::atomic<bool> g_stop{false};

void handle_signal(int) {
    g_stop.store(true, std::memory_order_release);
}

struct Args {
    std::string config_path{"configs/default.conf"};
    std::string out_dir{"artifacts"};
    std::uint64_t events{1'000'000};
};

Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto require_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for " + name);
            return argv[++i];
        };
        if (a == "--config") args.config_path = require_value(a);
        else if (a == "--out") args.out_dir = require_value(a);
        else if (a == "--events") args.events = std::stoull(require_value(a));
        else if (a == "--help") {
            std::cout << "Usage: llt_engine [--config configs/default.conf] [--out artifacts] [--events 1000000]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + a);
        }
    }
    return args;
}

void write_summary(const std::string& path, const LatencySummary& s, const Portfolio& pf, const Bbo& bbo, double elapsed_sec) {
    std::ofstream out(path);
    out << "run_completed_at=" << wall_clock_iso8601() << '\n';
    out << "events_processed=" << s.count << '\n';
    out << "elapsed_sec=" << elapsed_sec << '\n';
    out << "throughput_events_per_sec=" << (elapsed_sec > 0 ? static_cast<double>(s.count) / elapsed_sec : 0.0) << '\n';
    out << "latency_mean_ns=" << s.mean_ns << '\n';
    out << "latency_min_ns=" << s.min_ns << '\n';
    out << "latency_p50_ns=" << s.p50_ns << '\n';
    out << "latency_p90_ns=" << s.p90_ns << '\n';
    out << "latency_p99_ns=" << s.p99_ns << '\n';
    out << "latency_max_ns=" << s.max_ns << '\n';
    out << "fills=" << pf.fills() << '\n';
    out << "final_position=" << pf.position() << '\n';
    out << "final_cash=" << pf.cash() << '\n';
    out << "gross_notional=" << pf.gross_notional() << '\n';
    out << "final_pnl=" << pf.mark_to_market(bbo) << '\n';
}
} // namespace

int main(int argc, char** argv) {
    try {
        std::signal(SIGINT, handle_signal);
        const auto args = parse_args(argc, argv);
        Config cfg;
        cfg.load_file(args.config_path);
        fs::create_directories(args.out_dir);

        const std::string symbol = cfg.get_string("symbol", "SIM");
        SimulatedFeed feed({
            symbol,
            cfg.get_i64("initial_mid_ticks", 10'000),
            cfg.get_i64("base_spread_ticks", 2),
            cfg.get_i64("min_qty", 10),
            cfg.get_i64("max_qty", 500),
            static_cast<int>(cfg.get_i64("max_depth_levels", 12)),
            static_cast<std::uint64_t>(cfg.get_i64("seed", 42))
        });

        OrderBook book(symbol);
        MeanReversionStrategy strategy({
            static_cast<std::size_t>(cfg.get_i64("lookback", 64)),
            cfg.get_i64("threshold_ticks", 3),
            cfg.get_i64("order_qty", 25),
            static_cast<std::uint64_t>(cfg.get_i64("cooldown_events", 50))
        });
        RiskManager risk({
            cfg.get_i64("max_abs_position", 500),
            cfg.get_i64("max_order_qty", 100),
            cfg.get_double("max_gross_notional", 1'000'000.0)
        });
        OrderManager orders;
        Portfolio portfolio;
        Metrics metrics(static_cast<std::size_t>(args.events));
        TradeCsvWriter trades(args.out_dir + "/trades.csv");
        BookCsvWriter books(args.out_dir + "/book.csv");

        RedisClient redis;
        const bool use_redis = cfg.get_bool("use_redis", false);
        if (use_redis) {
            const auto ok = redis.connect_to(cfg.get_string("redis_host", "127.0.0.1"), static_cast<int>(cfg.get_i64("redis_port", 6379)));
            if (!ok) std::cerr << "Redis unavailable; continuing without Redis state publishing.\n";
        }

        constexpr std::size_t Q_CAP = 1u << 16u;
        SpscRing<MarketDataEvent, Q_CAP> queue;
        std::atomic<bool> producer_done{false};
        std::atomic<std::uint64_t> dropped{0};

        const auto start = std::chrono::steady_clock::now();

        std::thread producer([&] {
            for (std::uint64_t i = 0; i < args.events && !g_stop.load(std::memory_order_acquire); ++i) {
                auto ev = feed.next();
                while (!queue.push(ev)) {
                    dropped.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::yield();
                }
            }
            producer_done.store(true, std::memory_order_release);
        });

        std::thread engine([&] {
            MarketDataEvent ev{};
            std::uint64_t book_sample_every = static_cast<std::uint64_t>(cfg.get_i64("book_sample_every", 1000));
            if (book_sample_every == 0) book_sample_every = 1000;
            while (!producer_done.load(std::memory_order_acquire) || !queue.empty()) {
                if (!queue.pop(ev)) {
                    std::this_thread::yield();
                    continue;
                }
                const auto engine_enter = now_ns();
                book.apply(ev);
                const auto bbo = book.bbo();
                if (ev.seq % book_sample_every == 0) books.write(ev.seq, engine_enter, bbo);

                const auto before_strategy = now_ns();
                auto sig = strategy.on_bbo(ev.seq, ev.symbol, bbo, portfolio.position());
                const auto after_strategy = now_ns();

                if (sig) {
                    const auto decision = risk.check(*sig, portfolio.position(), portfolio.gross_notional());
                    if (decision.allowed) {
                        auto order = orders.from_signal(*sig);
                        auto exec = orders.try_execute(order, bbo);
                        if (exec) {
                            portfolio.on_fill(*exec);
                            const auto pnl = portfolio.mark_to_market(bbo);
                            trades.write(*exec, portfolio.cash(), portfolio.position(), pnl);
                            if (redis.connected() && exec->order_id % 20 == 0) {
                                redis.hset("llt:portfolio", "position", std::to_string(portfolio.position()));
                                redis.hset("llt:portfolio", "pnl", std::to_string(pnl));
                                redis.hset("llt:portfolio", "last_order_id", std::to_string(exec->order_id));
                            }
                        }
                    }
                }

                const auto done = now_ns();
                metrics.record({
                    ev.seq,
                    engine_enter - ev.exchange_ts_ns,      // queue delay / feed-to-engine handoff
                    after_strategy - before_strategy,       // strategy hot-path decision time
                    done - engine_enter                     // engine hot-path processing time
                });
            }
        });

        producer.join();
        engine.join();
        const auto end = std::chrono::steady_clock::now();
        const double elapsed_sec = std::chrono::duration<double>(end - start).count();

        metrics.write_csv(args.out_dir + "/latency.csv");
        const auto summary = metrics.summary();
        const auto final_bbo = book.bbo();
        write_summary(args.out_dir + "/summary.txt", summary, portfolio, final_bbo, elapsed_sec);

        std::cout << "Processed " << summary.count << " events in " << elapsed_sec << "s\n";
        std::cout << "Throughput: " << (elapsed_sec > 0 ? static_cast<double>(summary.count) / elapsed_sec : 0.0) << " events/s\n";
        std::cout << "Latency ns p50/p90/p99: " << summary.p50_ns << '/' << summary.p90_ns << '/' << summary.p99_ns << "\n";
        std::cout << "Fills: " << portfolio.fills() << ", position: " << portfolio.position()
                  << ", PnL: " << portfolio.mark_to_market(final_bbo) << "\n";
        std::cout << "Artifacts: " << args.out_dir << "\n";
        if (dropped.load() > 0) std::cerr << "Queue full retries: " << dropped.load() << "\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
}
