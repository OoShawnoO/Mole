/**
  ******************************************************************************
  * @file           : Mole.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/10
  ******************************************************************************
  */

#ifndef MOLE_H
#define MOLE_H

#include <string>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <deque>
#include <thread>

namespace hzd {
#ifdef _WIN32
    using ssize_t = long;
    using int8_t = char;
    using int16_t = short;
    using int32_t = int;
    using int64_t = long long int;
#endif
#define CACHE_BUF_SIZE 409600
    // 信号量
    class Semaphore {
    public:
        // 构造函数
        // 传入参数 count 表示初始信号量数量
        explicit Semaphore(ssize_t count = 0);
        // 析构函数
        ~Semaphore();
        // V操作
        // ++count
        void Signal();
        // V操作 唤醒所有阻塞
        // ++count
        void SignalAll();
        // P操作
        // 如果 count <= 0 阻塞 否则 --count
        void Wait();
    private:
        ssize_t                  count;
        std::mutex              mutex;
        std::condition_variable cv;
    };

    // 线程安全队列
    // T 对象类型
    template<typename T>
    class Channel {
    public:
        // block 是否阻塞等待
        // capacity 最大容量
        explicit Channel(bool block = false,size_t capacity = SIZE_MAX)
        :is_block(block),capacity(capacity){
            if(is_block) {
                shared_ptr_semaphore = std::make_shared<Semaphore>(0);
            }
        };
        Channel(Channel&& channel)  noexcept {
            container = std::move(channel.container);
            mutex = std::move(channel.mutex);
            shared_ptr_semaphore = channel.shared_ptr_semaphore;

            is_block = channel.is_block;
            capacity = channel.capacity;
        }
        // 唤醒阻塞
        void Push() {
            if(is_block) shared_ptr_semaphore->SignalAll();
        }
        // 对象入队 所有权转移
        bool Push(T&& item_right_ref){
            std::lock_guard<std::mutex> guard(mutex);
            if(container.size() >= capacity) return false;
            container.emplace_back(std::forward<T&&>(item_right_ref));
            if(is_block) shared_ptr_semaphore->Signal();
            return true;
        }
        // 对象入队
        bool Push(T& item_ref) {
            std::lock_guard<std::mutex> guard(mutex);
            if(container.size() >= capacity) return false;
            container.emplace_back(std::forward<T&>(item_ref));
            if(is_block) shared_ptr_semaphore->Signal();
            return true;
        }
        // 对象出队
        bool Pop(T& item_ref) {
            if(is_block) shared_ptr_semaphore->Wait();
            if(container.empty()) return false;
            std::lock_guard<std::mutex> guard(mutex);
            item_ref = std::move(container.front());
            container.pop_front();
            return true;
        }
        // 对象出队
        T Pop() {
            if(is_block) shared_ptr_semaphore->Wait();
            if(container.empty()) return {};
            auto ret = std::move(container.front());
            container.pop_front();
            return ret;
        }
        // 判空
        bool Empty() {
            std::lock_guard<std::mutex> guard(mutex);
            return container.empty();
        }
        // 当前大小
        size_t Size() {
            std::lock_guard<std::mutex> guard(mutex);
            return container.size();
        }
        // 最大容量
        inline size_t Capacity() {
            return capacity;
        }
    private:
        bool is_block;
        size_t capacity;
        std::deque<T> container;
        std::mutex mutex;
        std::shared_ptr<Semaphore> shared_ptr_semaphore;
    };

    // 解释方法
    // 简单变量字符串规则
    template<class T>
    struct has_stringfiy {
    private:
        template<class U>
        static auto has(int x) -> decltype(std::to_string(std::declval<U>()), std::true_type());
        template<class U>
        static std::false_type has(...);
    public:
        static constexpr bool value = decltype(has<T>(0))::value;
    };
    template<class T>
    struct is_string {
    private:
        template<class U>
        static auto has(int x) -> decltype(std::string(std::declval<U>()), std::true_type());
        template<class U>
        static std::false_type has(...);
    public:
        static constexpr bool value = decltype(has<T>(0))::value;
    };
    // iterator容器规则
    template<class T>
    struct has_iterator {
    private:
        template<class U>
        static auto has(int x) -> decltype(std::declval<U>().cbegin(), std::true_type());
        template<class U>
        static std::false_type has(...);
    public:
        static constexpr bool value = decltype(has<T>(0))::value;
    };
    // std::string规则
    template<class T,typename std::enable_if<is_string<T>::value,int>::type = 0>
    std::string Describe(const T& value) { return value; }
    // 限制字符串化规则
    template<class T,typename std::enable_if<has_stringfiy<T>::value,int>::type = 0>
    std::string Describe(const T& value) { return std::to_string(value); }
    // std::pair规则
    template<class K,class V,typename std::enable_if<!has_iterator<std::pair<K,V>>::value && !has_stringfiy<std::pair<K,V>>::value,int>::type = 0>
    std::string Describe(const std::pair<K,V>& value) { return "{" + Describe(value.first) + "," + Describe(value.second) + "}";}
    // 限制iterator规则
    template<class T,typename std::enable_if<!is_string<T>::value && has_iterator<T>::value,int>::type = 0>
    std::string Describe(const T& value) {
        std::string description = "{ ";
        for(auto iter = value.cbegin(); iter != value.cend(); ++iter) {
            description += Describe(*iter);
            description += " ";
        }
        description += "}";
        return description;
    }
    // 最基本引用规则
    template<class T,typename std::enable_if<!std::is_const<T>::value && !is_string<T>::value && !has_iterator<T>::value && !has_stringfiy<T>::value,int>::type = 0>
    std::string Describe(const T&) { return "__indescribable object__"; }

    #define MOLE_DEFINE(CLASS,object)                   \
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

    // 日志类
    class MOLE_API Mole {
    public:
        // 日志项
        struct LogItem {
            // 日志等级
            enum LogLevel {
                TRACE,INFO,WARN,ERROR_,FATAL
            };
            // 日志等级
            LogLevel                    level{TRACE};
            // 日志内容
            std::string                 content{};
            // 日志时间
            time_t                      time{};
            // 发布日志行号
            unsigned int                line{};
            // 发布日志文件位置
            const char*                 file_name{};
            // 日志打包变量内容
            std::string                 variables{};
            // 频道名
            std::string                 channel_name{};

            LogItem() = default;
            LogItem(
                    LogLevel level,
                    std::string                 content,
                    time_t                      time,
                    unsigned int                line,
                    const char*                 file_name,
                    std::string                 variables,
                    std::string                 channel_name
            );
            LogItem(LogItem&& log_item) noexcept;
            LogItem& operator=(LogItem&& log_item) noexcept;
        };
        // 日志频道
        class LogChannel {
        public:
            // 频道名称
            std::string         channel_name;
            // 屏蔽日志等级
            LogItem::LogLevel   level{LogItem::TRACE};
            // 提交队列
            Channel<LogItem>&    commit_channel;
            // 日志文件指针
            FILE*               fp{nullptr};
            // 日志写缓冲区
            char                write_buffer[CACHE_BUF_SIZE] = {0};
            // 日志写游标
            size_t              write_cursor{0};
            // 日志频道是否保存
            bool                is_save{false};
            // 日志频道是否终端输出
            bool                is_show{true};

            // 设置日志频道是否保存
            void SetSaveable(bool is_save);
            // 设置日志屏蔽等级
            void SetLevel(LogItem::LogLevel level);
            // 设置日志是否输出
            void SetShowLog(bool is_show);
            // 构造函数
            // channel_name 频道名
            explicit LogChannel(std::string  channel_name,Channel<LogItem>& commit_channel);
            ~LogChannel();

            // FATAL级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Fatal(const char* content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            void Fatal(const std::string& content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            // ERROR级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Error(const char* content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            void Error(const std::string& content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            // WARN级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Warn(const char* content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            void Warn(const std::string& content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            // INFO级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Info(const char* content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            void Info(const std::string& content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            // TRACE级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Trace(const char* content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            void Trace(const std::string& content,const char* file_name,unsigned int line,const std::vector<std::string>& variables = {}) noexcept;
            // 写日志项
            void WriteLogItem(LogItem& item);
        };
        // 获取日志频道
        static LogChannel& Channel(const std::string& channel_name = {});
        ~Mole();
        // 日志等级字符串哈希表
        const static std::unordered_map<std::string,LogItem::LogLevel>      level_str_map;
        // 是否关闭日志
        static bool                                                         is_disable;
        // 日志保存路径前缀
        static std::string                                                  save_path_prefix;
    private:
        // 控制日志线程关闭标记
        static bool                                                         is_stop;
        // 日志频道哈希表
        static std::unordered_map<std::string,std::shared_ptr<LogChannel>>  log_channel_map;
        // 日志线程组
        static std::vector<
            std::pair<
                std::thread,
                std::shared_ptr<hzd::Channel<LogItem>>
            >
        >                                                                   thread_group;
        // 对应thread_group挂载Channel<LogItem>的次数
        static std::vector<unsigned int>                                    commit_log_channel_count;
        // 日志等级哈希表
        const static std::unordered_map<LogItem::LogLevel,const char*>      level_map;
        // 日志等级颜色焊锡表
        const static std::unordered_map<LogItem::LogLevel,const char*>      color_scheme;
        // 日志循环
        static void LogLoop(const std::shared_ptr<hzd::Channel<LogItem>>& commit_channel);
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
        hzd::Mole::is_disable = bool;                                               \
    }while(0)
    // FATAL级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_FATAL(chan,content,...)                                                    \
    do {                                                                                \
        if(!hzd::Mole::is_disable){                                                     \
            auto& channel = hzd::Mole::Channel(chan);                                  \
            if(channel.level <= hzd::Mole::LogItem::LogLevel::FATAL){                   \
                channel.Fatal(content,__FILE__,__LINE__,##__VA_ARGS__);                 \
            }                                                                           \
        }                                                                               \
    }while(0)
    // ERROR级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_ERROR(chan,content,...)                                                    \
    do {                                                                                \
        if(!hzd::Mole::is_disable){                                                     \
            auto& channel = hzd::Mole::Channel(chan);                                  \
            if(channel.level <= hzd::Mole::LogItem::LogLevel::ERROR_){                  \
                channel.Error(content,__FILE__,__LINE__,##__VA_ARGS__);                 \
            }                                                                           \
        }                                                                               \
    }while(0)
    // WARN级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_WARN(chan,content,...)                                                     \
    do {                                                                                \
        if(!hzd::Mole::is_disable){                                                     \
            auto& channel = hzd::Mole::Channel(chan);                                  \
            if(channel.level <= hzd::Mole::LogItem::LogLevel::WARN){                    \
                channel.Warn(content,__FILE__,__LINE__,##__VA_ARGS__);                  \
            }                                                                           \
        }                                                                               \
    }while(0)
    // INFO级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_INFO(chan,content,...)                                                     \
    do {                                                                                \
        if(!hzd::Mole::is_disable){                                                     \
            auto& channel = hzd::Mole::Channel(chan);                                  \
            if(channel.level <= hzd::Mole::LogItem::LogLevel::INFO){                    \
                channel.Info(content,__FILE__,__LINE__,##__VA_ARGS__);                  \
            }                                                                           \
        }                                                                               \
    }while(0)
    // TRACE级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_VAR(var) 将在日志中显示var的运行时类型与数值
#define MOLE_TRACE(chan,content,...)                                                    \
    do {                                                                                \
        if(!hzd::Mole::is_disable){                                                     \
            auto& channel = hzd::Mole::Channel(chan);                                  \
            if(channel.level <= hzd::Mole::LogItem::LogLevel::TRACE){                   \
                channel.Trace(content,__FILE__,__LINE__,##__VA_ARGS__);                 \
            }                                                                           \
        }                                                                               \
    }while(0)
    // 隐式构造Variable
#define MOLE_VAR(variable) (std::string(#variable) + "=" + hzd::Describe(variable)+ "\n")
    // 设置日志频道最低等级
    // chan 日志频道名
    // level 等级字符串
#define MOLE_CHANNEL_LEVEL(chan,level)                                                  \
    do {                                                                                \
        if(!hzd::Mole::is_disable)                                                      \
            hzd::Mole::Channel(chan).SetLevel(hzd::Mole::level_str_map.at(level));     \
    }while(0)
    // 设置日志频道是否保存日志文件
    // chan 日志频道名
    // bool true保存 false不保存
#define MOLE_CHANNEL_SAVEABLE(chan,bool)                                                \
    do {                                                                                \
        if(!hzd::Mole::is_disable)                                                      \
            hzd::Mole::Channel(chan).SetSaveable(bool);                                \
    }while(0)

#define MOLE_CHANNEL_SHOWLOG(chan,bool)                                                 \
    do {                                                                                \
        if(!hzd::Mole::is_disable)                                                      \
            hzd::Mole::Channel(chan).SetShowLog(bool);                                 \
    }while(0)

#define MOLE_SAVE_PATH_PREFIX(path)                                                     \
    do {                                                                                \
        hzd::Mole::save_path_prefix = path;                                             \
    }while(0)

#endif
} // hzd

#endif //MOLE_H
