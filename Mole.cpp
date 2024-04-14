/**
  ******************************************************************************
  * @file           : Mole.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/10
  ******************************************************************************
  */

#include <iostream>
#include <utility>
#include <cstring>
#include <sstream>
#include "Mole.h"

#ifdef __linux__
#define CANCEL_COLOR_SCHEME "\033[0m"
#elif _WIN32
#include <windows.h>
#undef ERROR
#endif

namespace hzd {

    bool Mole::is_stop = false;
    std::unordered_map<std::string,std::shared_ptr<Mole::LogChannel>>  Mole::channel_map;
    std::vector<std::thread> Mole::thread_group;


    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::level_map {
            {Mole::LogItem::LogLevel::TRACE,"TRACE"},
            {Mole::LogItem::LogLevel::INFO,"INFO"},
            {Mole::LogItem::LogLevel::WARN,"WARN"},
            {Mole::LogItem::LogLevel::ERROR,"ERROR"},
            {Mole::LogItem::LogLevel::FATAL,"FATAL"}
    };

    const std::unordered_map<std::string,Mole::LogItem::LogLevel> Mole::level_str_map {
            {"TRACE",Mole::LogItem::LogLevel::TRACE},
            {"INFO",Mole::LogItem::LogLevel::INFO},
            {"WARN",Mole::LogItem::LogLevel::WARN},
            {"ERROR",Mole::LogItem::LogLevel::ERROR},
            {"FATAL",Mole::LogItem::LogLevel::FATAL}
    };

#ifdef __linux__
    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::color_scheme{
            {Mole::LogItem::LogLevel::TRACE,"\033[36m"},
            {Mole::LogItem::LogLevel::INFO,"\033[32m"},
            {Mole::LogItem::LogLevel::WARN,"\033[33m"},
            {Mole::LogItem::LogLevel::ERROR,"\033[31m"},
            {Mole::LogItem::LogLevel::FATAL,"\033[31m"}
    };
#elif _WIN32
    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::color_scheme{
            {Mole::LogItem::LogLevel::TRACE,"1"},
            {Mole::LogItem::LogLevel::INFO,"2"},
            {Mole::LogItem::LogLevel::WARN,"6"},
            {Mole::LogItem::LogLevel::ERROR,"12"},
            {Mole::LogItem::LogLevel::FATAL,"4"}
    };

    template<typename T>
    void ColorCout(T t,const int ForeColor=7,const int BackColor=0){
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),ForeColor+BackColor*0x10);
        std::cout << t;
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),7);
    }
#endif

    Semaphore::Semaphore(size_t count) : count(count) {

    }

    Semaphore::~Semaphore() {
        SignalAll();
    }

    void Semaphore::Signal() {
        std::unique_lock<std::mutex> guard(mutex);
        ++count;
        if(count <= 0) {
            cv.notify_one();
        }
    }

    void Semaphore::SignalAll() {
        std::unique_lock<std::mutex> guard(mutex);
        ++count;
        if(count <= 0) {
            cv.notify_all();
        }
    }

    void Semaphore::Wait() {
        std::unique_lock<std::mutex> guard(mutex);
        --count;
        if(count < 0) {
            cv.wait(guard);
        }
    }

    [[nodiscard]] std::string Mole::LogChannel::Variable::Detail() const {
        std::string data_string;
        switch(type) {
            case INT8: data_string = "type:int8_t value:" + std::to_string(data.int8_);
                break;
            case INT16: data_string = "type:int16_t value:" + std::to_string(data.int16_);
                break;
            case INT32: data_string = "type:int32_t value:" + std::to_string(data.int32_);
                break;
            case INT64: data_string = "type:int64_t value:" + std::to_string(data.int64_);
                break;
            case UINT8: data_string = "type:uint8_t value:" + std::to_string(data.uint8_);
                break;
            case UINT16: data_string = "type:uint16_t value:" + std::to_string(data.uint16_);
                break;
            case UINT32: data_string = "type:uint32_t value:" + std::to_string(data.uint32_);
                break;
            case UINT64: data_string = "type:uint64_t value:" + std::to_string(data.uint64_);
                break;
            case FLOAT: data_string = "type:float value:" + std::to_string(data.float_);
                break;
            case DOUBLE: data_string = "type:double value:" + std::to_string(data.double_);
                break;
            case LONG_DOUBLE: data_string = "type:long double value:" + std::to_string(data.ldouble_);
                break;
            case CONST_CHAR_PTR: data_string = "type:const char* value:" + std::string(data.constcharptr_);
                break;
            case CHAR_PTR: data_string = "type:char* value:" + std::string(data.charptr_);
                break;
        }
        return "name:" + std::string(name) + " " + data_string + "\n";
    }

    Mole::LogChannel::LogChannel(
            std::string channel_name
    ):channel_name(std::move(channel_name)){
    }



#define LOG_ITEM(type) do { \
        if(static_cast<int>(type) < level) return;              \
        std::string variable_string;                            \
        if(!variables.empty()){                                 \
            variable_string += "----------------------------\n";\
            for(const auto& v : variables ){                    \
                variable_string += v.Detail();                  \
            }                                                   \
            variable_string += "----------------------------\n";\
        }                                                       \
        LogItem item{type,content,std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),line,file_name,variable_string};\
        commit_channel.Push(std::move(item));                   \
}while(0)


    // FATAL级日志
    void Mole::LogChannel::Fatal(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        std::string _temp = content;
        Fatal(_temp,file_name,line,variables);
    }
    void Mole::LogChannel::Fatal(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        LOG_ITEM(LogItem::FATAL);
    }

    // ERROR级日志
    void Mole::LogChannel::Error(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        std::string _temp = content;
        Error(_temp,file_name,line,variables);
    }
    void Mole::LogChannel::Error(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        LOG_ITEM(LogItem::ERROR);
    }
    // WARN级日志
    void Mole::LogChannel::Warn(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        std::string _temp = content;
        Warn(_temp,file_name,line,variables);
    }
    void Mole::LogChannel::Warn(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        LOG_ITEM(LogItem::WARN);
    }
    // INFO级日志
    void Mole::LogChannel::Info(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        std::string _temp = content;
        Info(_temp,file_name,line,variables);
    }
    void Mole::LogChannel::Info(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        LOG_ITEM(LogItem::INFO);
    }
    // TRACE级日志
    void Mole::LogChannel::Trace(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        std::string _temp = content;
        Trace(_temp,file_name,line,variables);
    }
    void Mole::LogChannel::Trace(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<Variable>& variables
    ) noexcept {
        LOG_ITEM(LogItem::TRACE);
    }

    Mole::LogChannel &Mole::Channel(const std::string &channel_name) {
        static Mole mole;
        auto channel_map_iter = channel_map.find(channel_name);
        if(channel_map_iter == channel_map.end()) {
            channel_map[channel_name] = std::make_shared<LogChannel>(channel_name);
            auto& log_channel = channel_map.at(channel_name);
            thread_group.emplace_back(LogLoop,log_channel);
            return *log_channel;
        }
        else{
            return *channel_map_iter->second;
        }
    }

    void Mole::LogChannel::__WriteLogItem(Mole::LogItem &item) {
        char time_buffer[20] = {0};
        std::strftime(time_buffer,sizeof(time_buffer),"%Y-%m-%d %H:%M:%S",std::localtime(&item.time));
        memset(item_buffer,0,sizeof(item_buffer));

        if(item.content.size() + strlen(item.file_name) + channel_name.size()  > 4000 ) {
            std::cerr << "[WARN] single log data memory bigger than 4kb." << std::endl;
            std::stringstream stream;
            stream  << item.time
                    << " [ " << level_map.at(item.level) << "] "
                    << item.content
                    << " =>[" << channel_name << "] "
                    << "[" << item.file_name << ":"
                    << item.line << "]\n"
                    << item.variables;
            if(fp) {
                fwrite(write_buffer,write_cursor,1,fp);
                memset(write_buffer,0,sizeof(write_buffer));
                write_cursor = 0;
                fwrite(stream.str().c_str(),stream.str().size(),1,fp);
            }
#ifdef __linux__
            std::cout << color_scheme.at(item.level) << stream.str() << CANCEL_COLOR_SCHEME;
#elif _WIN32
            ColorCout(stream.str(),std::strtol(color_scheme.at(item.level),nullptr,10));
#endif
            fflush(stdout);
            return;
        }

        sprintf(
                item_buffer,
                "%s [%s] %s =>[%s] [%s:%d]\n%s",
                time_buffer,
                level_map.at(item.level),
                item.content.c_str(),
                channel_name.c_str(),
                item.file_name,
                item.line,
                item.variables.c_str()
        );
        if(fp) {
            strcat(write_buffer,item_buffer);
            write_cursor += strlen(item_buffer);
            if(write_cursor >= 4096 * 9) {
                fwrite(write_buffer,write_cursor,1,fp);
                memset(write_buffer,0,sizeof(write_buffer));
                write_cursor = 0;
            }
        }
#ifdef __linux__
        std::cout << color_scheme.at(item.level) << item_buffer << CANCEL_COLOR_SCHEME;
#elif _WIN32
        ColorCout(item_buffer,std::strtol(color_scheme.at(item.level),nullptr,10));
#endif
        fflush(stdout);


    }

    void Mole::LogChannel::SetSaveable(bool is_save_) {
        is_save = is_save_;
    }

    void Mole::LogChannel::SetLevel(Mole::LogItem::LogLevel _level) {
        level = _level;
    }

    void Mole::LogLoop(const std::shared_ptr<LogChannel>& log_channel) {
        LogChannel& chan = *log_channel;
        LogItem item{};
        if(chan.is_save) {
            chan.fp = fopen((std::string("log/") + chan.channel_name + ".log").c_str(),"a+");
            if(!chan.fp) std::cerr << "打开或创建log/" << chan.channel_name << "失败" << std::endl;
        }
        while(!is_stop) {
            if(!chan.commit_channel.Pop(item)) {
                break;
            }
            chan.__WriteLogItem(item);
        }
        while(!chan.commit_channel.Empty()) {
            if(chan.commit_channel.Pop(item)) {
                chan.__WriteLogItem(item);
            }
        }
        if(chan.fp) {
            if(strlen(chan.write_buffer) > 0) {
                    fwrite(chan.write_buffer,strlen(chan.write_buffer),1,chan.fp);
                    memset(chan.write_buffer,0,sizeof(chan.write_buffer));
                    chan.write_cursor = 0;
                }
            fclose(chan.fp);
            chan.fp = nullptr;
        }
    }

    Mole::~Mole() {
        is_stop = true;
        for(auto& channel_map_iter : channel_map) {
            channel_map_iter.second->commit_channel.Push();
        }
        for(auto& t : thread_group) {
            if(t.joinable()) t.join();
        }
        thread_group.clear();
    }
} // hzd