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

namespace hzd {
#define CANCEL_COLOR_SCHEME "\033[0m"

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

    const std::unordered_map<Mole::LogItem::LogLevel,const char*> Mole::color_scheme{
            {Mole::LogItem::LogLevel::TRACE,"\033[36m"},
            {Mole::LogItem::LogLevel::INFO,"\033[32m"},
            {Mole::LogItem::LogLevel::WARN,"\033[33m"},
            {Mole::LogItem::LogLevel::ERROR,"\033[31m"},
            {Mole::LogItem::LogLevel::FATAL,"\033[31m"}
    };

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

    Mole::LogChannel::LogChannel(
            std::string channel_name
    ):channel_name(std::move(channel_name)){
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
        bzero(item_buffer,sizeof(item_buffer));

        if(item.content.size() + strlen(item.file_name) + channel_name.size()  > 4000 ) {
            std::cerr << "[WARN] single log data memory bigger than 4kb." << std::endl;
            std::stringstream stream;
            stream  << "[" << item.time << "]"
                    << "[ " << channel_name << " ]"
                    << "[ " << level_map.at(item.level) << " ] "
                    << item.content
                    << "[" << item.file_name << ":"
                    << item.line << "]\n"
                    << item.variables;
            if(fp) {
                fwrite(write_buffer,write_cursor,1,fp);
                bzero(write_buffer,sizeof(write_buffer));
                write_cursor = 0;
                fwrite(stream.str().c_str(),stream.str().size(),1,fp);
            }
            std::cout << color_scheme.at(item.level) << stream.str() << CANCEL_COLOR_SCHEME;
            fflush(stdout);
            return;
        }

        sprintf(
                item_buffer,
                "[%s][%s][%s] %s [%s:%d]\n%s",
                time_buffer,
                level_map.at(item.level),
                channel_name.c_str(),
                item.content.c_str(),
                item.file_name,
                item.line,
                item.variables.c_str()
        );
        std::cout << color_scheme.at(item.level) << item_buffer << CANCEL_COLOR_SCHEME;
        fflush(stdout);

        if(fp) {
            strcat(write_buffer,item_buffer);
            write_cursor += strlen(item_buffer);
            if(write_cursor >= 4096 * 9) {
                fwrite(write_buffer,write_cursor,1,fp);
                bzero(write_buffer,sizeof(write_cursor));
                write_cursor = 0;
            }
        }
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
            chan.fp = fopen((std::string("log/") + chan.channel_name).c_str(),"a+");
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
                    bzero(chan.write_buffer,4096 * 10);
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
            t.join();
        }
        thread_group.clear();
    }
} // hzd