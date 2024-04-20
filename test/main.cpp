/**
  ******************************************************************************
  * @file           : main.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/10
  ******************************************************************************
  */
/* -if you want disable Mole log- */
/* 如果你希望关闭Mole的日志 */
//#define MOLE_CLOSE
/* ---------------------------- */
#include <unordered_map>
#include <vector>
#include <string>
#include "../Mole.h"

/* if you want custom class/struct to log,use macro MOLE_SELF_DEFINE() */
/* 想要自定义类型用于记录 可以使用MOLE_SELF_DEFINE()宏*/
struct Test {
    int x = 1;
    float y = 3.14;
    double z = 1.414;
};
MOLE_SELF_DEFINE(Test,test_object) {
    std::string temp = "\n";
    temp += MOLE_VAR(test_object.x);
    temp += MOLE_VAR(test_object.y);
    temp += MOLE_VAR(test_object.z);
    return temp;
}

int main() {
    /* if you dont want disable Mole before include Mole.h */
    /* 如果你不希望在include Mole.h前#define MOLE_CLOSE就关闭Mole日志功能 */
    /* 你可以使用如代码关闭*/
//    MOLE_DISABLE(true);

    /* set log channel whether save log file,default false */
    /* 设置日志频道是否保存日志文件,默认 false */
    MOLE_CHANNEL_SAVEABLE(channel_name,true);

    /* set log channel whether output log to terminal,default true */
    /* 设置日志频道是否输出日志到终端,默认开启 */
    MOLE_CHANNEL_SHOWLOG(channel_name,true);

    /* set log channel filter level */
    /* 设置日志频道过滤等级 */
    /* low to high TRACE -> INFO -> WARN -> ERROR -> FATAL */
    /* 由低到高 TRACE -> INFO -> WARN -> ERROR -> FATAL */
    MOLE_CHANNEL_LEVEL(channel_name,"TRACE");

    /* log */
    /* 打日志 */
    /* level trace */
    MOLE_TRACE(channel_name,"xxx");
    /* level info */
    MOLE_INFO(channel_name,"xxx");
    /* level warn */
    MOLE_WARN(channel_name,"xxx");
    /* level error */
    MOLE_ERROR(channel_name,"xxx");
    /* level fatal */
    MOLE_FATAL(channel_name,"xxx");

    int x = 1;
    float y = 3.14;
    double z = 1.414;
    const char* cstr = "hello world!";
    std::string stdstr = "hello world!";
    std::vector<int> vec = {1,2,3,4,5};
    std::unordered_map<int,std::string> umap  = { {1,"a"},{2,"b"} };
    /* 日志记录变量 */
    MOLE_INFO(channel_name,"xxx",{
            MOLE_VAR(x),MOLE_VAR(y),MOLE_VAR(z),MOLE_VAR(cstr),
            MOLE_VAR(stdstr),MOLE_VAR(vec),MOLE_VAR(umap)
    });

    /* MOLE_VAR() macro in order to describle variable,return std::string */
    /* MOLE_VAR() 宏将对变量进行解释,返回值是std::string */

    Test t{};

    MOLE_INFO(channel_name,"xxx",{ MOLE_VAR(t)});

    const Test ct{1,2.0,3.0};

    MOLE_INFO(channel_name,"xxx",{ MOLE_VAR(ct) });
}
