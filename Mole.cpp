/**
  ******************************************************************************
  * @file           : Mole.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/10
  ******************************************************************************
  */

#ifdef __linux__
#define CANCEL_COLOR_SCHEME "\033[0m"
#elif _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif
#include <iostream>
#include <utility>
#include <cstring>
#include <sstream>
#include "Mole.h"


namespace hzd {
    bool                                                                                Mole::is_disable = false;
    bool                                                                                Mole::is_stop = false;
    std::unordered_map<std::string,std::shared_ptr<Mole::LogChannel>>                   Mole::log_channel_map;
    std::vector<std::pair<std::thread,std::shared_ptr<hzd::Channel<Mole::LogItem>>>>    Mole::thread_group;
    std::vector<unsigned int>                                                           Mole::commit_log_channel_count;
    std::string                                                                         Mole::save_path_prefix{"log/"};
#ifdef _WIN32
    std::mutex                                                                          color_cout_lock;
#endif

    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::level_map {
            {Mole::LogItem::LogLevel::TRACE,"TRACE"},
            {Mole::LogItem::LogLevel::INFO,"INFO"},
            {Mole::LogItem::LogLevel::WARN,"WARN"},
            {Mole::LogItem::LogLevel::ERROR_,"ERROR"},
            {Mole::LogItem::LogLevel::FATAL,"FATAL"}
    };

    const std::unordered_map<std::string,Mole::LogItem::LogLevel> Mole::level_str_map {
            {"TRACE",Mole::LogItem::LogLevel::TRACE},
            {"INFO",Mole::LogItem::LogLevel::INFO},
            {"WARN",Mole::LogItem::LogLevel::WARN},
            {"ERROR",Mole::LogItem::LogLevel::ERROR_},
            {"FATAL",Mole::LogItem::LogLevel::FATAL}
    };

#ifdef __linux__

#define COLOR_RESET "\033[0m"
#define COLOR_BLACK "\033[30m"   /* Black */
#define COLOR_RED "\033[31m"    /* Red */
#define COLOR_GREEN "\033[32m"   /* Green */
#define COLOR_YELLOW "\033[33m"   /* Yellow */
#define COLOR_BLUE "\033[34m"   /* Blue */
#define COLOR_MAGENTA "\033[35m"   /* Magenta */
#define COLOR_CYAN "\033[36m"    /* Cyan */
#define COLOR_WHITE "\033[37m"   /* White */

#define COLOR_BOLD_BLACK "\033[1m\033[30m"   /* Bold Black */
#define COLOR_BOLD_RED "\033[1m\033[31m"   /* Bold Red */
#define COLOR_BOLD_GREEN "\033[1m\033[32m"  /* Bold Green */
#define COLOR_BOLD_YELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define COLOR_BOLD_BLUE "\033[1m\033[34m"   /* Bold Blue */
#define COLOR_BOLD_MAGENTA "\033[1m\033[35m"   /* Bold Magenta */
#define COLOR_BOLD_CYAN "\033[1m\033[36m"  /* Bold Cyan */
#define COLOR_BOLD_WHITE "\033[1m\033[37m"   /* Bold White */

    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::color_scheme{
            {Mole::LogItem::LogLevel::TRACE,COLOR_CYAN},
            {Mole::LogItem::LogLevel::INFO,COLOR_GREEN},
            {Mole::LogItem::LogLevel::WARN,COLOR_YELLOW},
            {Mole::LogItem::LogLevel::ERROR_,COLOR_RED},
            {Mole::LogItem::LogLevel::FATAL,COLOR_BOLD_RED}
    };
#elif _WIN32

#define COLOR_BLACK "0"   /* Black */
#define COLOR_RED "4"    /* Red */
#define COLOR_GREEN "2"   /* Green */
#define COLOR_YELLOW "6"   /* Yellow */
#define COLOR_BLUE "1"   /* Blue */
#define COLOR_MAGENTA "13"   /* Magenta */
#define COLOR_CYAN "9"    /* Cyan */
#define COLOR_WHITE "15"   /* White */

#define COLOR_BOLD_BLACK "0"   /* Bold Black */
#define COLOR_BOLD_RED "4"   /* Bold Red */
#define COLOR_BOLD_GREEN "2"  /* Bold Green */
#define COLOR_BOLD_YELLOW "6"  /* Bold Yellow */
#define COLOR_BOLD_BLUE "1"   /* Bold Blue */
#define COLOR_BOLD_MAGENTA "13"   /* Bold Magenta */
#define COLOR_BOLD_CYAN "9"  /* Bold Cyan */
#define COLOR_BOLD_WHITE "15"   /* Bold White */

    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::color_scheme{
            {Mole::LogItem::LogLevel::TRACE,"13"},
            {Mole::LogItem::LogLevel::INFO,COLOR_GREEN},
            {Mole::LogItem::LogLevel::WARN,COLOR_YELLOW},
            {Mole::LogItem::LogLevel::ERROR_,"12"},
            {Mole::LogItem::LogLevel::FATAL,COLOR_RED}
    };

    template<typename T>
    void ColorCout(T t,const int ForeColor=7,const int BackColor=0){
        color_cout_lock.lock();
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),ForeColor+BackColor*0x10);
        std::cout << t;
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),7);
        color_cout_lock.unlock();
    }
#endif

    Semaphore::Semaphore(ssize_t count) : count(count) {}

    Semaphore::~Semaphore() { SignalAll(); }

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

    Mole::LogItem::LogItem(
            LogLevel                    level_,
            std::string                 content_,
            time_t                      time_,
            unsigned int                line_,
            const char*                 file_name_,
            std::string                 variables_,
            std::string                 channel_name_
    ):  level(level_),content(std::move(content_)),time(time_),line(line_),file_name(file_name_),
        variables(std::move(variables_)),channel_name(std::move(channel_name_)){}

    Mole::LogItem::LogItem(Mole::LogItem &&log_item) noexcept{
        level = log_item.level;
        content = std::move(log_item.content);
        time = log_item.time;
        line = log_item.line;
        file_name = log_item.file_name;
        variables = std::move(log_item.variables);
        channel_name = std::move(log_item.channel_name);
    }

    Mole::LogItem &Mole::LogItem::operator=(Mole::LogItem &&log_item)  noexcept {
        level = log_item.level;
        content = std::move(log_item.content);
        time = log_item.time;
        line = log_item.line;
        file_name = log_item.file_name;
        variables = std::move(log_item.variables);
        channel_name = std::move(log_item.channel_name);
        return *this;
    }

    Mole::LogChannel::LogChannel(
            std::string channel_name_,
            hzd::Channel<Mole::LogItem>& commit_channel_
    ):channel_name(std::move(channel_name_)),commit_channel(commit_channel_){
        if(is_save) {
            fp = fopen((save_path_prefix + channel_name + ".log").c_str(),"a+");
            if(!fp) std::cerr << "open or create" << save_path_prefix << channel_name << ".log failed" << std::endl;
        }
    }

    void Mole::LogChannel::SetSaveable(bool is_save_) {
        if(!is_save && is_save_) {
            fp = fopen((save_path_prefix + channel_name + ".log").c_str(),"a+");
            if(!fp) std::cerr << "open or create"<< save_path_prefix << channel_name << ".log failed" << std::endl;
        }
        is_save = is_save_;
    }

    void Mole::LogChannel::SetShowLog(bool is_show_) { is_show = is_show_; }

    void Mole::LogChannel::SetLevel(Mole::LogItem::LogLevel _level) { level = _level; }

    Mole::LogChannel::~LogChannel() {
        if(fp) {
            if(strlen(write_buffer) > 0) {
                fwrite(write_buffer,strlen(write_buffer),1,fp);
                memset(write_buffer,0,sizeof(write_buffer));
                write_cursor = 0;
            }
            fclose(fp);
            fp = nullptr;
        }
    }



#define LOG_ITEM(type) do {                                                                                         \
        std::string variable_string;                                                                                \
        if(!variables.empty()){                                                                                     \
            variable_string += "----------------------------\n";                                                    \
            for(auto& v : variables ){                                                                        \
                variable_string += std::move(v);                                                                    \
            }                                                                                                       \
            variable_string += "----------------------------\n";                                                    \
        }                                                                                                           \
        LogItem item{type,content,std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),line,file_name,variable_string,channel_name};\
        commit_channel.Push(std::move(item));                                                                       \
}while(0)


    // FATAL级日志
    void Mole::LogChannel::Fatal(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        Fatal(std::string { content },file_name,line,variables);
    }
    void Mole::LogChannel::Fatal(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        LOG_ITEM(LogItem::FATAL);
    }

    // ERROR级日志
    void Mole::LogChannel::Error(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        Error(std::string { content },file_name,line,variables);
    }
    void Mole::LogChannel::Error(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        LOG_ITEM(LogItem::ERROR_);
    }
    // WARN级日志
    void Mole::LogChannel::Warn(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        Warn(std::string { content },file_name,line,variables);
    }
    void Mole::LogChannel::Warn(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        LOG_ITEM(LogItem::WARN);
    }
    // INFO级日志
    void Mole::LogChannel::Info(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        Info(std::string { content },file_name,line,variables);
    }
    void Mole::LogChannel::Info(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        LOG_ITEM(LogItem::INFO);
    }
    // TRACE级日志
    void Mole::LogChannel::Trace(
            const char* content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        Trace(std::string { content },file_name,line,variables);
    }
    void Mole::LogChannel::Trace(
            const std::string& content,
            const char* file_name,
            unsigned int line,
            const std::vector<std::string>& variables
    ) noexcept {
        LOG_ITEM(LogItem::TRACE);
    }

    Mole::LogChannel &Mole::Channel(const std::string &channel_name) {
        static Mole mole;
        auto channel_map_iter = log_channel_map.find(channel_name);
        if(channel_map_iter == log_channel_map.end()) {
            std::shared_ptr<hzd::Channel<LogItem>> commit_channel_ptr;
            if(thread_group.empty() || commit_log_channel_count.back() > 2) {
                commit_channel_ptr = std::make_shared<hzd::Channel<LogItem>>(true);
                thread_group.emplace_back(std::thread{LogLoop,commit_channel_ptr},commit_channel_ptr);
                commit_log_channel_count.emplace_back(1);
            }else {
                commit_channel_ptr = thread_group.back().second;
                ++commit_log_channel_count.back();
            }
            auto log_channel = std::make_shared<LogChannel>(channel_name,*commit_channel_ptr);
            log_channel_map[channel_name] = log_channel;
            return *log_channel;
        }
        else{
            return *channel_map_iter->second;
        }
    }

    void Mole::LogChannel::WriteLogItem(Mole::LogItem &item) {
        char time_buffer[20] = {0};
        std::strftime(time_buffer,sizeof(time_buffer),"%Y-%m-%d %H:%M:%S",std::localtime(&item.time));

        std::string log_item_str;
        log_item_str += time_buffer;
        log_item_str += " [";log_item_str += level_map.at(item.level);log_item_str += "] ";
        log_item_str += item.content;
        log_item_str += " => [";log_item_str += channel_name;log_item_str += "] [";
        log_item_str += item.file_name;log_item_str += ":";log_item_str +=  std::to_string(item.line) += "]\n";
        log_item_str += item.variables;

        if(fp) {
            if(write_cursor + log_item_str.size() >= CACHE_BUF_SIZE) {
                fwrite(write_buffer,write_cursor,1,fp);
                memset(write_buffer,0,sizeof(write_buffer));
                write_cursor = 0;
            }
            strcat(write_buffer,log_item_str.c_str());
            write_cursor += log_item_str.size();
        }

        if(!is_show) return;
#ifdef __linux__
        std::cout << color_scheme.at(item.level) << log_item_str << CANCEL_COLOR_SCHEME;
#elif _WIN32
        ColorCout(log_item_str,std::strtol(color_scheme.at(item.level),nullptr,10));
#endif
        fflush(stdout);
    }


    void Mole::LogLoop(const std::shared_ptr<hzd::Channel<LogItem>>& commit_channel) {
        hzd::Channel<LogItem>& chan = *commit_channel;
        LogItem item{};

        while(!is_stop) {
            if(!chan.Pop(item)) {
                break;
            }
            auto log_channel = log_channel_map.at(item.channel_name);
            log_channel->WriteLogItem(item);
        }
        while(!chan.Empty()) {
            if(chan.Pop(item)) {
                auto log_channel = log_channel_map.at(item.channel_name);
                log_channel->WriteLogItem(item);
            }
        }
    }

    Mole::~Mole() {
        is_stop = true;
        for(auto& t : thread_group) {
            t.second->Push();
            if(t.first.joinable()) t.first.join();
        }
    }

} // hzd