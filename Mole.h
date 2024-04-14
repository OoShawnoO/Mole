/**
  ******************************************************************************
  * @file           : Mole.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/10
  ******************************************************************************
  */

#ifndef MOLE_MOLE_H
#define MOLE_MOLE_H

#include <deque>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <thread>

namespace hzd {
    // 信号量
    class Semaphore {
    public:
        // 构造函数
        // 传入参数 count 表示初始信号量数量
        explicit Semaphore(size_t count = 0);
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
        size_t                  count;
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
            shared_ptr_semaphore = std::move(channel.shared_ptr_semaphore);
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
#define STR(x) #x
    // 日志类
    class Mole {
    public:
        // 日志项
        struct LogItem {
            // 日志等级
            enum LogLevel {
                TRACE,INFO,WARN,ERROR,FATAL
            };
            // 日志等级
            LogLevel        level;
            // 日志内容
            std::string     content;
            // 日志时间
            time_t          time;
            // 发布日志行号
            unsigned int    line;
            // 发布日志文件位置
            const char*     file_name;
            // 日志打包变量内容
            std::string     variables;
        };
        // 日志频道
        class LogChannel {
        public:
            // 频道名称
            std::string         channel_name;
            // 屏蔽日志等级
            LogItem::LogLevel   level{LogItem::TRACE};
            // 提交队列
            Channel<LogItem>    commit_channel{true};
            // 日志文件指针
            FILE*               fp{nullptr};
            // 日志项缓冲区
            char                item_buffer[4096] = {0};
            // 日志写缓冲区
            char                write_buffer[40960] = {0};
            // 日志写游标
            size_t              write_cursor{0};
            // 日志频道是否保存
            bool                is_save{false};

            // 设置日志频道是否保存
            void SetSaveable(bool is_save);
            // 设置日志屏蔽等级
            void SetLevel(LogItem::LogLevel level);
            // 构造函数
            // channel_name 频道名
            explicit LogChannel(std::string  channel_name);

#ifdef _WIN32
    using int8_t = char;
    using int16_t = short;
    using int32_t = int;
    using int64_t = long long;
#endif

            // 变量包
            struct Variable {
                const char* name{nullptr};
                union data{
                    int8_t int8_;
                    int16_t int16_;
                    int32_t int32_;
                    int64_t int64_;
                    uint8_t uint8_;
                    uint16_t uint16_;
                    uint32_t uint32_;
                    uint64_t uint64_;
                    double double_;
                    long double ldouble_;
                    float float_;
                    char* charptr_;
                    const char* constcharptr_;
                } data{};
                enum VariableType {
                    INT8,INT16,INT32,INT64,
                    UINT8,UINT16,UINT32,UINT64,
                    FLOAT,DOUBLE,LONG_DOUBLE,
                    CONST_CHAR_PTR,CHAR_PTR,
                } type;
                Variable(const char* n,int8_t v){ name = n;type = INT8; data.int8_ = v;};
                Variable(const char* n,int16_t v){ name = n;type = INT16; data.int16_ = v; };
                Variable(const char* n,int32_t v){ name = n;type = INT32; data.int32_ = v; };
                Variable(const char* n,int64_t v){ name = n;type = INT64; data.int64_ = v; };
                Variable(const char* n,uint8_t v){ name = n;type = UINT8; data.uint8_ = v; };
                Variable(const char* n,uint16_t v){ name = n;type = UINT16; data.uint16_ = v; };
                Variable(const char* n,uint32_t v){ name = n;type = UINT32; data.uint32_ = v; };
                Variable(const char* n,uint64_t v){ name = n;type = UINT64; data.uint64_ = v; };
                Variable(const char* n,float v){ name = n;type = FLOAT; data.float_ = v; };
                Variable(const char* n,double v){ name = n;type = DOUBLE; data.double_ = v; };
                Variable(const char* n,long double v){ name = n;type = LONG_DOUBLE; data.ldouble_ = v; };
                Variable(const char* n,const char* v){ name = n;type = CONST_CHAR_PTR; data.constcharptr_ = v; };
                Variable(const char* n,char* v){ name = n;type = CHAR_PTR; data.charptr_ = v; };

                // 描述变量名称、类型与数值
                [[nodiscard]] std::string Detail() const;
            };

            // FATAL级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Fatal(const char* content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            void Fatal(const std::string& content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            // ERROR级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Error(const char* content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            void Error(const std::string& content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            // WARN级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Warn(const char* content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            void Warn(const std::string& content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            // INFO级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Info(const char* content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            void Info(const std::string& content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            // TRACE级别日志
            // content 日志内容
            // file_name 发布日志位置文件名
            // line 发布日志位置行号
            // variables 变量包
            void Trace(const char* content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            void Trace(const std::string& content,const char* file_name,unsigned int line,const std::vector<Variable>& variables = {}) noexcept;
            // 写日志项
            void __WriteLogItem(LogItem& item);
        };
        // 获取日志频道
        static LogChannel& Channel(const std::string& channel_name = {});
        ~Mole();
        // 日志等级字符串哈希表
        const static std::unordered_map<std::string,LogItem::LogLevel>      level_str_map;
    private:
        // 控制日志线程关闭标记
        static bool                                                         is_stop;
        // 日志频道哈希表
        static std::unordered_map<std::string,std::shared_ptr<LogChannel>>  channel_map;
        // 日志线程组
        static std::vector<std::thread>                                     thread_group;
        // 日志等级哈希表
        const static std::unordered_map<LogItem::LogLevel,const char*>      level_map;
        // 日志等级颜色焊锡表
        const static std::unordered_map<LogItem::LogLevel,const char*>      color_scheme;
        // 日志循环
        static void LogLoop(const std::shared_ptr<LogChannel>& log_channel);
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
    // FATAL级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_PACK(var) 将在日志中显示var的运行时类型与数值
#define MOLE_FATAL(chan,content,...)                                             \
    do {                                                                      \
        hzd::Mole::Channel(#chan).Fatal(content,__FILE__,__LINE__,##__VA_ARGS__);\
    }while(0)
    // ERROR级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_PACK(var) 将在日志中显示var的运行时类型与数值
#define MOLE_ERROR(chan,content,...)                                          \
    do {                                                                      \
        hzd::Mole::Channel(#chan).Error(content,__FILE__,__LINE__,##__VA_ARGS__);\
    }while(0)
    // WARN级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_PACK(var) 将在日志中显示var的运行时类型与数值
#define MOLE_WARN(chan,content,...)                                          \
    do {                                                                      \
        hzd::Mole::Channel(#chan).Warn(content,__FILE__,__LINE__,##__VA_ARGS__);\
    }while(0)
    // INFO级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_PACK(var) 将在日志中显示var的运行时类型与数值
#define MOLE_INFO(chan,content,...)                                          \
    do {                                                                      \
        hzd::Mole::Channel(#chan).Info(content,__FILE__,__LINE__,##__VA_ARGS__);\
    }while(0)
    // TRACE级别日志
    // chan 日志频道名
    // content 日志内容
    // 可变参数 使用 MOLE_PACK(var) 将在日志中显示var的运行时类型与数值
#define MOLE_TRACE(chan,content,...)                                          \
    do {                                                                      \
        hzd::Mole::Channel(#chan).Trace(content,__FILE__,__LINE__,##__VA_ARGS__);\
    }while(0)
    // 隐式构造Variable
#define MOLE_VAR(variable) {#variable,variable}
    // 设置日志频道最低等级
    // chan 日志频道名
    // level 等级字符串
#define MOLE_CHANNEL_LEVEL(chan,level) \
    do {                               \
        hzd::Mole::Channel(#chan).SetLevel(hzd::Mole::level_str_map.at(level));        \
    }while(0)
    // 设置日志频道是否保存日志文件
    // chan 日志频道名
    // bool true保存 false不保存
#define MOLE_CHANNEL_SAVEABLE(chan,bool) \
    do {                                 \
        hzd::Mole::Channel(#chan).SetSaveable(bool);                                     \
    }while(0)

#endif
} // hzd

#endif //MOLE_MOLE_H
