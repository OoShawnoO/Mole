/**
  ******************************************************************************
  * @file           : Mole.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/7/13
  ******************************************************************************
  */

#ifndef MOLE_H
#define MOLE_H


#include <cstdint>
#include <cstdio>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <memory>
#include <vector>
#include <unordered_map>

namespace hzd {

#if defined(WIN32) || defined(_WIN32)
    using int8_t    = char;
    using int16_t   = short;
    using int32_t   = int;
    using int64_t   = long;
    using uint8_t   = unsigned char;
    using uint16_t  = unsigned short;
    using uint32_t  = unsigned int;
    using size_t    = unsigned int;
    using ssize_t   = long int;
#endif // WIN32

    namespace internal {
        /**
         * @brief Semaphore 信号量
         * @details 信号量是一种特殊的变量，用于控制对共享资源的访问。
         */
        class Semaphore {
        public:
            /**
             * constructor
             * @param count 信号量初始值
             */
            explicit Semaphore(ssize_t count = 0);

            /**
             * @brief 信号量加一, 唤醒一个等待的线程
             */
            void Signal();

            /**
             * @brief 唤醒所有等待线程
             */
            void Wake();

            /**
             * @brief 等待信号量, 阻塞线程, 直到信号量大于0
             */
            void Wait();

        private:
            ssize_t count;
            std::mutex mutex;
            std::condition_variable cv;
        };

        template<class T>
        struct Chan {
            explicit Chan(bool is_block = false, size_t capacity = 1024 * 1024 * 16 / sizeof(T))
                    : capacity(capacity) {
                if (is_block) {
                    sp_sem = std::make_shared<Semaphore>(0);
                }
            }

            Chan(Chan &&chan) noexcept {
                capacity = chan.capacity;
                mutex = std::move(chan.mutex);
                sp_sem = std::move(chan.sp_sem);
                container = std::move(chan.container);

            }

            // 唤醒阻塞线程
            void Wake() {
                if (sp_sem) {
                    sp_sem->Wake();
                }
            }

            // 放入对象
            bool Push(T &&value) {
                std::lock_guard<std::mutex> guard(mutex);
                if (container.size() >= capacity) return false;
                container.emplace_back(std::forward<T>(value));
                if (sp_sem) sp_sem->Signal();
                return true;
            }

            // 放入对象
            bool operator<<(T &&value) {
                std::lock_guard<std::mutex> guard(mutex);
                if (container.size() >= capacity) return false;
                container.emplace_back(std::forward<T>(value));
                if (sp_sem) sp_sem->Signal();
                return true;
            }

            // 获取对象
            bool Pop(T &value) {
                if (sp_sem) sp_sem->Wait();
                if (container.empty()) return false;
                std::lock_guard<std::mutex> guard(mutex);
                value = std::move(container.front());
                container.pop_front();
                return true;
            }

            // 获取对象
            bool operator>>(T &value) {
                if (sp_sem) sp_sem->Wait();
                if (container.empty()) return false;
                std::lock_guard<std::mutex> guard(mutex);
                value = std::move(container.front());
                container.pop_front();
                return true;
            }

            // 是否为空
            bool IsEmpty() const {
                return container.empty();
            }

            // 是否已满
            bool IsFull() const {
                return container.size() >= capacity;
            }

            // 获取当前对象数量
            size_t Size() const {
                return container.size();
            }

            // 获取容量
            size_t Capacity() const {
                return capacity;
            }

            Chan(const Chan &chan) = delete;

            Chan &operator=(const Chan &) = delete;

        private:
            size_t capacity;
            std::mutex mutex;
            std::deque<T> container;
            std::shared_ptr<Semaphore> sp_sem;
        };
    }

    // 解释方法
    // 简单变量字符串规则
    template<class T>
    struct to_string_able {
    private:
        template<class U>
        __attribute__((unused)) static auto
        has(int) -> decltype(std::to_string(std::declval<U>()), std::true_type()) { return {}; };

        template<class>
        static std::false_type has(...) { return {}; };
    public:
        static constexpr bool value = decltype(has<T>(0))::value;
    };

    template<class T>
    struct is_string {
    private:
        template<class U>
        __attribute__((unused)) static auto
        has(int) -> decltype(std::string(std::declval<U>()), std::true_type()) { return {}; };

        template<class>
        static std::false_type has(...) { return {}; };
    public:
        static constexpr bool value = decltype(has<T>(0))::value;
    };

    // iterator容器规则
    template<class T>
    struct has_iterator {
    private:
        template<class U>
        static auto has(int) -> decltype(std::declval<U>().cbegin(), std::true_type()) { return {}; };

        template<class>
        static std::false_type has(...) { return {}; };
    public:
        static constexpr bool value = decltype(has<T>(0))::value;
    };

    // std::string规则
    template<class T, typename std::enable_if<is_string<T>::value, int>::type = 0>
    std::string Describe(const T &value) { return value; }

    // 限制字符串化规则
    template<class T, typename std::enable_if<to_string_able<T>::value, int>::type = 0>
    std::string Describe(const T &value) { return std::to_string(value); }

    // std::pair规则
    template<class K, class V, typename std::enable_if<
            !has_iterator<std::pair<K, V>>::value && !to_string_able<std::pair<K, V>>::value, int>::type = 0>
    std::string Describe(const std::pair<K, V> &value) {
        return "{" + Describe(value.first) + "," + Describe(value.second) + "}";
    }

    // 限制iterator规则
    template<class T, typename std::enable_if<!is_string<T>::value && has_iterator<T>::value, int>::type = 0>
    std::string Describe(const T &value) {
        std::string description = "{ ";
        for (auto iter = value.cbegin(); iter != value.cend(); ++iter) {
            description += Describe(*iter);
            description += " ";
        }
        description += "}";
        return description;
    }

    // 最基本引用规则
    template<class T, typename std::enable_if<
            !std::is_const<T>::value && !is_string<T>::value && !has_iterator<T>::value &&
            !to_string_able<T>::value, int>::type = 0>
    std::string Describe(const T &) { return "__indescribable object__"; }

#define MOLE_DEFINE(CLASS, object)                   \
    template<>                                          \
    std::string hzd::Describe<CLASS>(const CLASS& object)

#ifdef _WIN32
#ifdef MOLE_EXPORTS
#define MOLE_API __declspec(dllexport)
#else
#define MOLE_API __declspec(dllimport)
#endif
#else
#define MOLE_API
#endif

#define CACHE_BUF_SIZE (16 * 1024)

    class MOLE_API Mole {

        class Channel;

    public:
        enum class Level : uint8_t {
            SILENCE = 0,    // 静默级日志
            TRACE,          // 追踪级日志
            DEBUG,          // 调试级日志
            INFO,           // 信息级日志
            WARN,           // 警告级日志
            ERROR,          // 错误级日志
            FATAL           // 严重错误级日志
        };

        ~Mole();

        // 追踪日志
        static void trace(
                std::string content,
                const char *file = nullptr,
                uint32_t line = 0,
                std::vector<std::string> &&vars = {}
        );

        // 通知日志
        static void info(
                std::string content,
                const char *file = nullptr,
                uint32_t line = 0,
                std::vector<std::string> &&vars = {}
        );

        // 调试日志
        static void debug(
                std::string content,
                const char *file = nullptr,
                uint32_t line = 0,
                std::vector<std::string> &&vars = {}
        );

        // 警告日志
        static void warn(
                std::string content,
                const char *file = nullptr,
                uint32_t line = 0,
                std::vector<std::string> &&vars = {}
        );

        // 错误日志
        static void error(
                std::string content,
                const char *file = nullptr,
                uint32_t line = 0,
                std::vector<std::string> &&vars = {}
        );

        // 严重错误日志
        static void fatal(
                std::string content,
                const char *file = nullptr,
                uint32_t line = 0,
                std::vector<std::string> &&vars = {}
        );

        // 获取日志频道
        static Channel &channel(const std::string &name);

        // 设置日志保存前缀路径
        static void save_prefix(const std::string &prefix);

        // 设置日志为可用
        static void enable(bool is_enable);

        // 设置所有日志控制台输出状态
        static void console(bool is_console);

    private:
        struct Meta {
            // 日志等级
            Level level{Level::SILENCE};
            // 日志内容
            std::string content{};
            // 日志时间
            time_t time{std::time(nullptr)};
            // 日志文件名
            const char *file{nullptr};
            // 日志行号
            uint32_t line{0};
            // 日志线程ID
            std::thread::id tid{std::this_thread::get_id()};
            // 日志变量包
            std::vector<std::string> vars{};
            // 日志频道
            Channel *channel{nullptr};


            Meta() = default;

            Meta(
                    Level level,
                    std::string content,
                    time_t time,
                    const char *file,
                    uint32_t line,
                    std::thread::id tid,
                    std::vector<std::string> vars,
                    Channel *channel
            );

            Meta(Meta &&meta) noexcept;

            Meta &operator=(Meta &&meta) noexcept;
        };

        using Chan = internal::Chan<Meta>;

        class Channel {
            friend class Mole;

        public:
            Channel();

            ~Channel() = default;

            // 追踪日志
            void trace(
                    std::string content,
                    const char *file = nullptr,
                    uint32_t line = 0,
                    std::vector<std::string> &&vars = {}
            );

            // 通知日志
            void info(
                    std::string content,
                    const char *file = nullptr,
                    uint32_t line = 0,
                    std::vector<std::string> &&vars = {}
            );

            // 调试日志
            void debug(
                    std::string content,
                    const char *file = nullptr,
                    uint32_t line = 0,
                    std::vector<std::string> &&vars = {}
            );

            // 警告日志
            void warn(
                    std::string content,
                    const char *file = nullptr,
                    uint32_t line = 0,
                    std::vector<std::string> &&vars = {}
            );

            // 错误日志
            void error(
                    std::string content,
                    const char *file = nullptr,
                    uint32_t line = 0,
                    std::vector<std::string> &&vars = {}
            );

            // 严重错误日志
            void fatal(
                    std::string content,
                    const char *file = nullptr,
                    uint32_t line = 0,
                    std::vector<std::string> &&vars = {}
            );

            // 日志过滤级别设置
            void SetLevel(Level level);

            // 日志保存设置
            void SetSave(bool is_save);

            // 日志输出设置
            void SetConsole(bool is_console);

            // 设置日志通道名
            void SetName(std::string name);

            // 设置日志存储路径
            void SetPath(std::string path);

        private:
            // 日志通道名
            std::string name{};
            // 日志存储路径
            std::string path{};
            // 日志过滤等级
            Level level{Level::SILENCE};
            // 日志文件指针
            FILE *fp{nullptr};
            // 日志写游标
            size_t write_cursor{0};
            // 日志写缓冲区
            char write_buffer[CACHE_BUF_SIZE]{};
            // 是否允许本通道保存文件
            bool is_save{false};
            // 是否允许本通道输出到控制台
            bool is_console{true};

            // 写日志
            void writeMeta(Meta &&meta);

            // 打开日志文件
            bool openLogFile();

            // 日志接口
            void log(
                    Level level,
                    std::string content,
                    const char *file = __FILE__,
                    uint32_t line = __LINE__,
                    std::vector<std::string> &&vars = {}
            );
        };

        // 日志结束标志
        static bool is_stop;
        // 日志可用标志
        static bool is_enable;
        // 日志控制台输出标志
        static bool is_console;
        // 日志队列
        static Chan chan;
        // 日志线程
        static std::thread thread;
        // 日志文件保存前缀
        static std::string prefix;
        // 日志通道映射
        static std::unordered_map<std::string, Mole::Channel> channels;

        // 日志loop
        static void loop();

        // 日志接口
        static void log(
                Level level,
                std::string content,
                const char *file = __FILE__,
                uint32_t line = __LINE__,
                std::vector<std::string> &&vars = {}
        );
    };

#ifdef MOLE_CLOSE
#define MOLE_FATAL(chan,content,...)
#define MOLE_ERROR(chan,content,...)
#define MOLE_WARN(chan,content,...)
#define MOLE_INFO(chan,content,...)
#define MOLE_TRACE(chan,content,...)
#define MOLE_VAR(variable)
#define MOLE_CHANNEL_LEVEL(chan,level)
#define MOLE_CHANNEL_SAVEABLE(chan,bool)
#else
#define MOLE_DISABLE(bool)                                                          \
    do {                                                                            \
        if(bool)                                                                    \
            hzd::Mole::disable();                                                   \
        else                                                                        \
            hzd::Mole::enable();                                                    \
    }while(0)
    // FATAL级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_FATAL(chan, content, ...)                                                    \
    do {                                                                                \
        hzd::Mole::channel(chan).fatal(content,__FILE__,__LINE__,##__VA_ARGS__);                                                                            \
    }while(0)
    // ERROR级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_ERROR(chan, content, ...)                                                    \
    do {                                                                                \
        hzd::Mole::channel(chan).error(content,__FILE__,__LINE__,##__VA_ARGS__);                                                                             \
    }while(0)
    // WARN级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_WARN(chan, content, ...)                                                     \
    do {                                                                                \
        hzd::Mole::channel(chan).warn(content,__FILE__,__LINE__,##__VA_ARGS__);                                                                           \
    }while(0)
    // INFO级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_INFO(chan, content, ...)                                                     \
    do {                                                                                \
        hzd::Mole::channel(chan).info(content,__FILE__,__LINE__,##__VA_ARGS__);                                                                           \
    }while(0)
    // DEBUG级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_DEBUG(chan, content, ...)                                                    \
    do {                                                                                \
        hzd::Mole::channel(chan).debug(content,__FILE__,__LINE__,##__VA_ARGS__);                                                                          \
    }while(0)
    // TRACE级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_TRACE(chan, content, ...)                                                    \
    do {                                                                                \
        hzd::Mole::channel(chan).trace(content,__FILE__,__LINE__,##__VA_ARGS__);                                                                          \
    }while(0)
    // 隐式构造Variable
#define MOLE_VAR(variable) (std::string(#variable) + "=" + hzd::Describe(variable))
    // 设置日志频道最低等级
    // chan 日志频道名
    // level 等级字符串
#define MOLE_CHANNEL_LEVEL(chan, level)                                                  \
    do {                                                                                \
            hzd::Mole::channel(chan).SetLevel(level);                                   \
    }while(0)
    // 设置日志频道是否保存日志文件
    // chan 日志频道名
    // bool true保存 false不保存
#define MOLE_CHANNEL_SAVEABLE(chan, bool)                                                \
    do {                                                                                \
        hzd::Mole::channel(chan).SetSave(bool);                                     \
    }while(0)

#define MOLE_CHANNEL_SHOWLOG(chan, bool)                                                 \
    do {                                                                                \
        hzd::Mole::channel(chan).SetConsole(bool);                                      \
    }while(0)

#define MOLE_SAVE_PATH_PREFIX(path)                                                     \
    do {                                                                                \
        hzd::Mole::save_prefix(path);   \
    }while(0)

#endif


} // hzd

#endif //MOLE_H
