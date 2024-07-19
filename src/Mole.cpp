/**
  ******************************************************************************
  * @file           : Mole.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/7/13
  ******************************************************************************
  */

#include "Mole.h"
#include "fmt/format.h"
#include "fmt/color.h"

#include <utility>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace hzd {
#undef ERROR

    internal::Semaphore::Semaphore(ssize_t count) : count(count) {}

    void internal::Semaphore::Signal() {
        std::unique_lock<std::mutex> guard(mutex);
        ++count;
        if (count >= 0) {
            cv.notify_one();
        }
    }

    void internal::Semaphore::Wake() {
        std::unique_lock<std::mutex> guard(mutex);
        ++count;
        if (count >= 0) {
            cv.notify_all();
        }
    }

    void internal::Semaphore::Wait() {
        std::unique_lock<std::mutex> guard(mutex);
        --count;
        if (count < 0) {
            cv.wait(guard);
        }
    }


    Mole::Meta::Meta(Mole::Meta &&meta) noexcept {
        level = meta.level;
        content = std::move(meta.content);
        time = meta.time;
        line = meta.line;
        file = meta.file;
        tid = meta.tid;
        vars = std::move(meta.vars);
        channel = meta.channel;
    }

    Mole::Meta &Mole::Meta::operator=(Mole::Meta &&meta) noexcept {
        level = meta.level;
        content = std::move(meta.content);
        time = meta.time;
        line = meta.line;
        file = meta.file;
        tid = meta.tid;
        vars = std::move(meta.vars);
        channel = meta.channel;
        return *this;
    }

    Mole::Meta::Meta(
            Mole::Level level,
            std::string content,
            time_t time,
            const char *file,
            uint32_t line,
            std::thread::id tid,
            std::vector<std::string> vars,
            Mole::Channel *channel
    )
            : level(level), content(std::move(content)), time(time), file(file), line(line), tid(tid),
              vars(std::move(vars)), channel(channel) {}


    Mole::Channel::Channel() = default;

    void Mole::Channel::trace(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        if (level > Mole::Level::TRACE || !is_enable) return;
        log(Mole::Level::TRACE, std::move(content), file, line, std::move(vars));
    }

    void Mole::Channel::info(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        if (level > Mole::Level::INFO || !is_enable) return;
        log(Mole::Level::INFO, std::move(content), file, line, std::move(vars));
    }

    void Mole::Channel::debug(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        if (level > Mole::Level::DEBUG || !is_enable) return;
        log(Mole::Level::DEBUG, std::move(content), file, line, std::move(vars));
    }

    void Mole::Channel::warn(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        if (level > Mole::Level::WARN || !is_enable) return;
        log(Mole::Level::WARN, std::move(content), file, line, std::move(vars));
    }

    void Mole::Channel::error(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        if (level > Mole::Level::ERROR || !is_enable) return;
        log(Mole::Level::ERROR, std::move(content), file, line, std::move(vars));
    }

    void Mole::Channel::fatal(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        if (!is_enable) return;
        log(Mole::Level::FATAL, std::move(content), file, line, std::move(vars));
    }

    void Mole::Channel::log(
            Mole::Level level_,
            std::string content,
            const char *file,
            uint32_t line,
            std::vector<std::string> &&vars
    ) {
        Meta meta(level_, std::move(content), time(nullptr), file, line, std::this_thread::get_id(), std::move(vars),
                  this);
        chan.Push(std::move(meta));
    }

    void Mole::Channel::SetLevel(Mole::Level level_) {
        level = level_;
    }

    void Mole::Channel::SetSave(bool is_save_) {
        is_save = is_save_;
    }

    void Mole::Channel::SetConsole(bool is_console_) {
        is_console = is_console_;
    }

    void Mole::Channel::writeMeta(Mole::Meta &&meta) {
        if (!meta.channel->is_console && !meta.channel->is_save) return;

        char time_str[20] = {0};
        std::strftime(
                time_str,
                sizeof(time_str),
                "%Y-%m-%d %H:%M:%S",
                std::localtime(&meta.time)
        );

        std::stringstream ss;
        ss << meta.tid;
        auto tid = ss.str();

        std::string var_str;
        for (auto &&var: meta.vars) {
            var_str += "[variable] " + var + "\n";
        }

        std::string meta_str = fmt::format(
                "{} [{:^10}] {} [{}] [{}:{} thread:{}]\n{}",
                time_str,
                meta.level == Mole::Level::TRACE ? "TRACE" :
                meta.level == Mole::Level::INFO ? "INFO" :
                meta.level == Mole::Level::DEBUG ? "DEBUG" :
                meta.level == Mole::Level::WARN ? "WARN" :
                meta.level == Mole::Level::ERROR ? "ERROR" :
                meta.level == Mole::Level::FATAL ? "FATAL" : "",
                meta.content,
                meta.channel->name,
                meta.file,
                meta.line,
                tid,
                var_str
        );

        if (meta.channel->is_save) {
            if (meta.channel->fp == nullptr) {
                meta.channel->fp = fopen((prefix + meta.channel->name + ".log").c_str(), "ab");
                if (meta.channel->fp == nullptr) {
                    fmt::print("Failed to open file: {}", meta.channel->name + ".log");
                }
            }
            if (write_cursor + meta_str.size() >= CACHE_BUF_SIZE) {
                fwrite(write_buffer, write_cursor, 1, fp);
                memset(write_buffer, 0, sizeof(write_buffer));
                write_cursor = 0;
            }
            strcat(write_buffer, meta_str.c_str());
            write_cursor += meta_str.size();
        }

        if (meta.channel->is_console) {

            fmt::println(
                    "{} [{:^7}] {} |{}| [{}:{} thread:{}]",
                    time_str,
                    fmt::styled(
                            (
                                    meta.level == Mole::Level::TRACE ? "TRACE" :
                                    meta.level == Mole::Level::INFO ? "INFO" :
                                    meta.level == Mole::Level::DEBUG ? "DEBUG" :
                                    meta.level == Mole::Level::WARN ? "WARN" :
                                    meta.level == Mole::Level::ERROR ? "ERROR" :
                                    meta.level == Mole::Level::FATAL ? "FATAL" : ""
                            ),
                            (
                                    meta.level == Mole::Level::TRACE ? fmt::fg(fmt::color::black) |
                                                                       fmt::bg(fmt::color::gray) :
                                    meta.level == Mole::Level::INFO ? fmt::fg(fmt::color::black) |
                                                                      fmt::bg(fmt::color::green) :
                                    meta.level == Mole::Level::DEBUG ? fmt::fg(fmt::color::black) |
                                                                       fmt::bg(fmt::color::blue) :
                                    meta.level == Mole::Level::WARN ? fmt::fg(fmt::color::black) |
                                                                      fmt::bg(fmt::color::yellow) :
                                    meta.level == Mole::Level::ERROR ? fmt::fg(fmt::color::black) |
                                                                       fmt::bg(fmt::color::red) :
                                    meta.level == Mole::Level::FATAL ? fmt::fg(fmt::color::black) |
                                                                       fmt::bg(fmt::color::magenta) :
                                    fmt::fg(fmt::color::black) | fmt::bg(fmt::color::white)
                            )
                    ),
                    meta.content,
                    fmt::styled(meta.channel->name, fmt::fg(fmt::color::black) | fmt::bg(fmt::color::green)),
                    meta.file,
                    meta.line,
                    tid
            );

            for (auto &var: meta.vars) {
                fmt::println("[ variable ] {}",
                             fmt::styled(var, fmt::fg(fmt::color::black) | fmt::bg(fmt::color::dark_cyan)));
            }


        }

    }

    void Mole::Channel::SetName(std::string name_) { name = std::move(name_); }

    bool Mole::is_stop = false;
    bool Mole::is_enable = true;
    std::unordered_map<std::string, Mole::Channel> Mole::channels;
    Mole::Chan Mole::chan(true);
    std::string Mole::prefix = "./";
    std::thread Mole::thread(loop);
    static Mole mole;

    Mole::~Mole() {
        is_stop = true;
        chan.Wake();
        thread.join();
    }

    void Mole::loop() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
        Meta meta;
        while (!is_stop) {
            if (!chan.Pop(meta)) {
                break;
            }
            meta.channel->writeMeta(std::move(meta));
        }
        while (!chan.IsEmpty()) {
            if (chan.Pop(meta)) {
                meta.channel->writeMeta(std::move(meta));
            }
        }
        for (auto &channel: hzd::Mole::channels) {
            if (channel.second.fp != nullptr) {
                if (channel.second.write_cursor != 0) {
                    fwrite(channel.second.write_buffer, channel.second.write_cursor, 1, channel.second.fp);
                    channel.second.write_cursor = 0;
                }
                fclose(channel.second.fp);
            }
        }
    }

    void Mole::trace(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        log(Level::TRACE, std::move(content), file, line, std::move(vars));
    }

    void Mole::info(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        log(Level::INFO, std::move(content), file, line, std::move(vars));
    }

    void Mole::debug(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        log(Level::DEBUG, std::move(content), file, line, std::move(vars));
    }

    void Mole::warn(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        log(Level::WARN, std::move(content), file, line, std::move(vars));
    }

    void Mole::error(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        log(Level::ERROR, std::move(content), file, line, std::move(vars));
    }

    void Mole::fatal(std::string content, const char *file, uint32_t line, std::vector<std::string> &&vars) {
        log(Level::FATAL, std::move(content), file, line, std::move(vars));
    }

    Mole::Channel &Mole::channel(const std::string &name) {
        auto &channel_ref = channels[name];
        if (channel_ref.name.empty()) {
            channel_ref.SetName(name);
        }
        return channel_ref;
    }

    void Mole::save_prefix(const std::string &prefix_) {
        Mole::prefix = prefix_;
    }

    void Mole::enable() { Mole::is_enable = true; }

    void Mole::disable() { Mole::is_enable = false; }

    void Mole::log(
            Mole::Level level,
            std::string content,
            const char *file,
            uint32_t line,
            std::vector<std::string> &&vars
    ) {
        if (!Mole::is_enable) { return; }
        static auto &logger = channel("main");
        logger.log(level, std::move(content), file, line, std::move(vars));
    }

#define ERROR 0
} // hzd