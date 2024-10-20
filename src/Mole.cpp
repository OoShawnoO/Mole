/**
  ******************************************************************************
  * @file           : Mole.cc
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/10/20
  ******************************************************************************
  */

#include <unordered_map>
#include <regex>
#include <utility>
#include <iomanip>
#include <iostream>
#include "Mole.h"
#include "fmt/color.h"

#ifdef _WIN32
#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <regex>
#endif

namespace hzd {

static std::unordered_map<Mole::Level, std::string> level_map{
    {Mole::Level::kSILENCE, "SILENCE"},
    {Mole::Level::kTRACE, "TRACE"},
    {Mole::Level::kDEBUG, "DEBUG"},
    {Mole::Level::kINFO, "INFO"},
    {Mole::Level::kWARN, "WARN"},
    {Mole::Level::kERROR, "ERROR"},
    {Mole::Level::kFATAL, "FATAL"},
};

static std::unordered_map<Mole::Level, fmt::color> color_schema{
    {Mole::Level::kSILENCE, fmt::color::green_yellow},
    {Mole::Level::kTRACE, fmt::color::cyan},
    {Mole::Level::kDEBUG, fmt::color::blue},
    {Mole::Level::kINFO, fmt::color::green},
    {Mole::Level::kWARN, fmt::color::yellow},
    {Mole::Level::kERROR, fmt::color::red},
    {Mole::Level::kFATAL, fmt::color::dark_red},
};

Mole::Mole() {
  stop_ = false;
}

Mole::~Mole() {
  stop_ = true;
  meta_chan_.Wake();
  thread_.join();
}

void Mole::Log(Mole::Meta &&meta) {
  if (!enable_ || meta.op == kNONE && meta.level < filter) return;
  meta.time = std::chrono::system_clock::now();
  meta.thread_id = std::this_thread::get_id();
  meta_chan_.Push(std::move(meta));
}
void Mole::Enable(bool is_enable) {
  Log(Meta{Level::kSILENCE, is_enable ? Operation::kENABLE : Operation::kDISABLE});
}
void Mole::Console(bool is_console) {
  Log(Meta{Level::kSILENCE, is_console ? Operation::kCONSOLE : Operation::kNCONSOLE});
}
void Mole::Save(bool is_save, std::string path) {
  Log(Meta{Level::kSILENCE, is_save ? Operation::kSAVE : Operation::kNSAVE, std::move(path)});
}
void Mole::LogFilter(Mole::Level level) {
  Log(Meta{level, Operation::kFILTER});
}

void Mole::loop(Mole *m) {
#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#endif
  Meta meta;
  Mole &mole = *m;
  while (!mole.stop_) {
    if (!mole.meta_chan_.Pop(meta)) { continue; }
    mole.writeMeta(std::move(meta));
  }
  while (!mole.meta_chan_.Empty()) {
    if (mole.meta_chan_.Pop(meta)) {
      mole.writeMeta(std::move(meta));
    }
  }
  if (mole.fp) {
    fwrite(mole.buffer, mole.cursor, 1, mole.fp);
    mole.cursor = 0;
    fclose(mole.fp);
    mole.fp = nullptr;
  }

}
void Mole::writeMeta(Mole::Meta &&meta) {
  switch (meta.op) {
    case kNONE : {
      if (!enable_ || !console_ && !save_) return;

      auto duration = meta.time.time_since_epoch();
      auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
      auto t_count = std::chrono::system_clock::to_time_t(meta.time);

      std::stringstream time_stream, tid_stream;
      time_stream << std::put_time(std::localtime(&t_count), "%Y-%m-%d %X")
                  << '.' << std::setfill('0') << std::setw(6) << microseconds % 1000000;
      tid_stream << meta.thread_id;

      auto styled_str = fmt::format(
          "{} [{:^7}] {} [{}:{} thread:{}]\n",
          time_stream.str(),
          fmt::styled(level_map[meta.level], fmt::bg(fmt::color::black) | fmt::fg(color_schema[meta.level])),
          meta.content,
          meta.file,
          meta.line,
          tid_stream.str()
      );

      auto raw_str = std::regex_replace(styled_str, std::regex("\033\\[[0-9;]*m"), "");

      if (console_) {
        fmt::print(styled_str);
      }

      if (save_) {
        if (!fp) {
          fp = fopen(save_path_.c_str(), "ab");
          if (!fp) {
            save_ = false;
            Log(Meta{Level::kFATAL, kNONE, fmt::format("open log file:{} failed!", meta.content), {}, __LINE__,
                     __FILE__,
                     {}});
            return;
          }
        }

        if (cursor + raw_str.size() >= CACHE_BUF_SIZE) {
          fwrite(buffer, cursor, 1, fp);
          buffer[0] = '\0';
          cursor = 0;
        }
        if (raw_str.size() > CACHE_BUF_SIZE) {
          fwrite(raw_str.c_str(), raw_str.size(), 1, fp);
        } else {
          strcat(buffer, raw_str.c_str());
          cursor += raw_str.size();
        }
      }

      break;
    }
    case kENABLE: {
      enable_ = true;
      break;
    }
    case kDISABLE: {
      enable_ = false;
      break;
    }
    case kCONSOLE: {
      console_ = true;
      break;
    }
    case kNCONSOLE: {
      console_ = false;
      break;
    }
    case kSAVE: {
      save_ = true;
      if (save_path_ != meta.content) {
        if (fp) {
          fclose(fp);
          fp = nullptr;
        }
        fp = fopen(meta.content.c_str(), "ab");
        if (!fp) {
          save_ = false;
          Log(Meta{Level::kFATAL, kNONE, fmt::format("open log file:{} failed!", meta.content), {}, __LINE__, __FILE__,
                   {}});
        }
      }
      break;
    }
    case kNSAVE: {
      save_ = false;
      break;
    }
    case kFILTER: {
      filter = meta.level;
      break;
    }
  }
}
Mole &Mole::Instance() {
  static Mole mole;
  return mole;
}

Mole::Meta::Meta(Mole::Level level_,
                 Mole::Operation op_,
                 std::string content_,
                 std::thread::id thread_id_,
                 uint32_t line_,
                 const char *file_,
                 Mole::time_point time_) :
    level(level_), op(op_), content(std::move(content_)), thread_id(thread_id_), line(line_), file(file_), time(time_) {

}

} // hzd