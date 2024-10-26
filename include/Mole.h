/**
  ******************************************************************************
  * @file           : Mole.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/10/20
  ******************************************************************************
  */

#ifndef MOLE_MOLE_H
#define MOLE_MOLE_H

#include <condition_variable>
#include <deque>
#include <memory>
#include <shared_mutex>
#include <thread>

#include "fmt/format.h"
#include "concurrent/blockingconcurrentqueue.h"

namespace hzd {

#if defined(WIN32) || defined(_WIN32)
using int8_t = char;
using int16_t = short;
using int32_t = int;
using int64_t = long;
using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using size_t = unsigned long long;
using ssize_t = long int;
#ifdef MOLE_EXPORTS
#define MOLE_API __declspec(dllexport)
#else
#define MOLE_API __declspec(dllimport)
#endif // MOLE_EXPORTS
#define FILENAME(x) (strrchr(x,'\\') ? strrchr(x,'\\')+1 : x)
#else  // NOT WIN32 || _WIN32 end
#define MOLE_API
#define FILENAME(x) (strrchr(x, '/') ? strrchr(x, '/')+1 : x)
#endif // WIN32 || _WIN32

#define MB (1024*1024)  // M Bytes
#define CACHE_BUF_SIZE (16 * MB)
#define META_BULK_SIZE 16

class MOLE_API Mole {
 public:
  template<class T>
  using Chan = moodycamel::BlockingConcurrentQueue<T>;

  enum class Level : uint32_t {
    kTRACE = 0, kINFO, kDEBUG, kWARN, kERROR, kFATAL, kSILENCE,
  };
  enum Operation {
    kNONE, kENABLE, kDISABLE, kCONSOLE, kNCONSOLE, kSAVE, kNSAVE, kFILTER
  };
  using time_point = std::chrono::system_clock::time_point;
  struct Meta {
    Level level{Level::kSILENCE};
    Operation op{kNONE};
    std::string content{};
    std::thread::id thread_id{};
    uint32_t line{0};
    const char *file{nullptr};
    time_point time{};

    explicit Meta(Level level = Level::kSILENCE,
                  Operation op = kNONE,
                  std::string content = "",
                  std::thread::id thread_id = {},
                  uint32_t line = 0,
                  const char *file = nullptr,
                  time_point time = {});
  };

  Mole();
  ~Mole();
  void Log(Meta &&meta);
  void Enable(bool is_enable);
  void Console(bool is_console);
  void Save(bool is_save, std::string path = "Mole.log");
  void LogFilter(Level level);
  static Mole &Instance();

 private:
  bool enable_ = true, console_ = true, save_ = false, stop_ = false;
  Level filter{Level::kTRACE};
  std::thread thread_{loop, this};
  Chan<Meta> meta_chan_;

  std::string save_path_;
  FILE *fp{};
  char buffer[CACHE_BUF_SIZE]{};
  size_t cursor{};

  static void loop(Mole *mole);
  void writeMeta(Meta &&meta);

};

#if defined(WIN32) || defined(_WIN32)
#pragma warning(pop)
#endif

#ifdef MOLE_IGNORE
#define MOLE_TRACE(str,...)
#define MOLE_DEBUG(str,...)
#define MOLE_INFO(str,...)
#define MOLE_WARN(str,...)
#define MOLE_ERROR(str,...)
#define MOLE_FATAL(str,...)
#define MOLE_LEVEL(level)
#define MOLE_ENABLE(is_enable)
#define MOLE_SAVE(is_save,...)
#define MOLE_CONSOLE(is_console)
#else
#define MOLE_TRACE(str, ...) do { \
  hzd::Mole::Instance().Log(hzd::Mole::Meta{hzd::Mole::Level::kTRACE,hzd::Mole::Operation::kNONE,fmt::format(str,##__VA_ARGS__), {},__LINE__,FILENAME(__FILE__),{}}); \
}while(0)
#define MOLE_DEBUG(str, ...) do { \
  hzd::Mole::Instance().Log(hzd::Mole::Meta{hzd::Mole::Level::kDEBUG,hzd::Mole::Operation::kNONE,fmt::format(str,##__VA_ARGS__), {},__LINE__,FILENAME(__FILE__),{}}); \
}while(0)
#define MOLE_INFO(str, ...) do { \
  hzd::Mole::Instance().Log(hzd::Mole::Meta{hzd::Mole::Level::kINFO,hzd::Mole::Operation::kNONE,fmt::format(str,##__VA_ARGS__), {},__LINE__,FILENAME(__FILE__),{}}); \
}while(0)
#define MOLE_WARN(str, ...) do { \
  hzd::Mole::Instance().Log(hzd::Mole::Meta{hzd::Mole::Level::kWARN,hzd::Mole::Operation::kNONE,fmt::format(str,##__VA_ARGS__), {},__LINE__,FILENAME(__FILE__),{}}); \
}while(0)
#define MOLE_ERROR(str, ...) do { \
  hzd::Mole::Instance().Log(hzd::Mole::Meta{hzd::Mole::Level::kERROR,hzd::Mole::Operation::kNONE,fmt::format(str,##__VA_ARGS__), {},__LINE__,FILENAME(__FILE__),{}}); \
}while(0)
#define MOLE_FATAL(str, ...) do { \
  hzd::Mole::Instance().Log(hzd::Mole::Meta{hzd::Mole::Level::kFATAL,hzd::Mole::Operation::kNONE,fmt::format(str,##__VA_ARGS__), {},__LINE__,FILENAME(__FILE__),{}}); \
}while(0)
#define MOLE_LEVEL(level) do { \
  hzd::Mole::Instance().LogFilter(level);\
}while(0)
#define MOLE_ENABLE(is_enable) do { \
  hzd::Mole::Instance().Enable(is_enable);\
}while(0)
#define MOLE_SAVE(is_save, ...) do { \
  hzd::Mole::Instance().Save(is_save,##__VA_ARGS__);\
}while(0)
#define MOLE_CONSOLE(is_console) do { \
  hzd::Mole::Instance().Console(is_console);                                      \
}while(0)
#endif

} // hzd
#endif //MOLE_MOLE_H
