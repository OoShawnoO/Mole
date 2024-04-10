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
        struct LogItem {
            enum LogLevel {
                TRACE,INFO,WARN,ERROR,FATAL
            };
            LogLevel        level;
            std::string     content;
            time_t          time;
            unsigned int    line;
            const char*     file_name;
            std::string     variables;
        };
        class LogChannel {
        public:
            std::string         channel_name;
            LogItem::LogLevel   level{LogItem::TRACE};
            Channel<LogItem>    commit_channel{true};
            FILE*               fp{nullptr};
            char                item_buffer[4096] = {0};
            char                write_buffer[40960] = {0};
            size_t              write_cursor{0};
            bool                is_save{false};

            void SetSaveable(bool is_save);
            void SetLevel(LogItem::LogLevel level);
            explicit LogChannel(std::string  channel_name);

            struct Variable {
                const char* name{nullptr};
                union data{
                    int8_t _int8;
                    int16_t _int16;
                    int32_t _int32;
                    int64_t _int64;
                    uint8_t _uint8;
                    uint16_t _uint16;
                    uint32_t _uint32;
                    uint64_t _uint64;
                    double _double;
                    long double _ldouble;
                    float _float;
                    char* _charptr;
                    const char* _constcharptr;
                } data{};
                enum VariableType {
                    INT8,INT16,INT32,INT64,
                    UINT8,UINT16,UINT32,UINT64,
                    FLOAT,DOUBLE,LONG_DOUBLE,
                    CONST_CHAR_PTR,CHAR_PTR,
                } type;
                Variable(const char* n,int8_t v){ name = n;type = INT8; data._int8 = v;};
                Variable(const char* n,int16_t v){ name = n;type = INT16; data._int16 = v; };
                Variable(const char* n,int32_t v){ name = n;type = INT32; data._int32 = v; };
                Variable(const char* n,int64_t v){ name = n;type = INT64; data._int64 = v; };
                Variable(const char* n,uint8_t v){ name = n;type = UINT8; data._uint8 = v; };
                Variable(const char* n,uint16_t v){ name = n;type = UINT16; data._uint16 = v; };
                Variable(const char* n,uint32_t v){ name = n;type = UINT32; data._uint32 = v; };
                Variable(const char* n,uint64_t v){ name = n;type = UINT64; data._uint64 = v; };
                Variable(const char* n,float v){ name = n;type = FLOAT; data._float = v; };
                Variable(const char* n,double v){ name = n;type = DOUBLE; data._double = v; };
                Variable(const char* n,long double v){ name = n;type = LONG_DOUBLE; data._ldouble = v; };
                Variable(const char* n,const char* v){ name = n;type = CONST_CHAR_PTR; data._constcharptr = v; };
                Variable(const char* n,char* v){ name = n;type = CHAR_PTR; data._charptr = v; };

                [[nodiscard]] std::string Detail() const {
                    std::string data_string;
                    switch(type) {
                        case INT8: data_string = "type:int8_t value:" + std::to_string(data._int8);
                            break;
                        case INT16: data_string = "type:int16_t value:" + std::to_string(data._int16);
                            break;
                        case INT32: data_string = "type:int32_t value:" + std::to_string(data._int32);
                            break;
                        case INT64: data_string = "type:int64_t value:" + std::to_string(data._int64);
                            break;
                        case UINT8: data_string = "type:uint8_t value:" + std::to_string(data._uint8);
                            break;
                        case UINT16: data_string = "type:uint16_t value:" + std::to_string(data._uint16);
                            break;
                        case UINT32: data_string = "type:uint32_t value:" + std::to_string(data._uint32);
                            break;
                        case UINT64: data_string = "type:uint64_t value:" + std::to_string(data._uint64);
                            break;
                        case FLOAT: data_string = "type:float value:" + std::to_string(data._float);
                            break;
                        case DOUBLE: data_string = "type:double value:" + std::to_string(data._double);
                            break;
                        case LONG_DOUBLE: data_string = "type:long double value:" + std::to_string(data._ldouble);
                            break;
                        case CONST_CHAR_PTR: data_string = "type:const char* value:" + std::string(data._constcharptr);
                            break;
                        case CHAR_PTR: data_string = "type:char* value:" + std::string(data._charptr);
                            break;
                    }
                    return "name:" + std::string(name) + " " + data_string + "\n";
                }
            };

#define LOG_ITEM(type) do { \
        if(static_cast<int>(type) < level) return;              \
        std::string varibale_string;                            \
        if(!variables.empty()){                                 \
            for(const auto& v : variables ){                    \
                varibale_string += v.Detail();                  \
            }                                                   \
        }                                                       \
        LogItem item{type,content,std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),line,file_name,varibale_string};\
        commit_channel.Push(std::move(item));                   \
}while(0)

            // FATAL级日志
            void Fatal(
                    const char* content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                std::string _temp = content;
                Fatal(_temp,file_name,line,variables);
            }
            void Fatal(
                    const std::string& content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                LOG_ITEM(LogItem::FATAL);
            }
            
            // ERROR级日志
            void Error(
                    const char* content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                std::string _temp = content;
                Error(_temp,file_name,line,variables);
            }
            void Error(
                    const std::string& content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                LOG_ITEM(LogItem::ERROR);
            }
            // WARN级日志
            void Warn(
                    const char* content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                std::string _temp = content;
                Warn(_temp,file_name,line,variables);
            }
            void Warn(
                    const std::string& content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                LOG_ITEM(LogItem::WARN);
            }
            // INFO级日志
            void Info(
                    const char* content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                std::string _temp = content;
                Info(_temp,file_name,line,variables);
            }
            void Info(
                    const std::string& content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                LOG_ITEM(LogItem::INFO);
            }
            // TRACE级日志
            void Trace(
                    const char* content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                std::string _temp = content;
                Trace(_temp,file_name,line,variables);
            }
            void Trace(
                    const std::string& content,
                    const char* file_name,
                    unsigned int line,
                    const std::vector<Variable>& variables = {}
            ) noexcept {
                LOG_ITEM(LogItem::TRACE);
            }

            void __WriteLogItem(LogItem& item);
        };
        // 获取日志频道
        static LogChannel& Channel(const std::string& channel_name = {});
        ~Mole();
        const static std::unordered_map<std::string,LogItem::LogLevel>      level_str_map;
    private:
        static bool                                                         is_stop;
        static std::unordered_map<std::string,std::shared_ptr<LogChannel>>  channel_map;
        static std::vector<std::thread>                                     thread_group;

        const static std::unordered_map<LogItem::LogLevel,const char*>      level_map;
        const static std::unordered_map<LogItem::LogLevel,const char*>      color_scheme;

        static void LogLoop(const std::shared_ptr<LogChannel>& log_channel);
    };

#ifdef MOLE_CLOSE
#define MOLE_FATAL(chan,content,...)
#define MOLE_ERROR(chan,content,...)
#define MOLE_WARN(chan,content,...)
#define MOLE_INFO(chan,content,...)
#define MOLE_TRACE(chan,content,...)
#define MOLE_PACK(variable)
#define MOLE_CHANNEL_LEVEL(chan,level)
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
#define MOLE_PACK(variable) {#variable,variable}
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
