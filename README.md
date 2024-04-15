# Mole
## 介绍 Introduce
Mole是一个异步日志库，提供日志频道记录日志的功能。

Mole is an asynchronous logging library that provides log channel recording functionality.

日志频道是日志控制主体，不同的日志频道可以设置不同的日志过滤等级、日志保存与否、日志是否输出在终端等，日志的输出文件路径也不同。

The log channel is the subject of log control. Different log channels can set different log filtering levels, whether to save logs, whether to output logs to the terminal, etc. The output file paths of logs are also different.

值得一提的是Mole库提供了便捷的变量记录功能，当需要在日志中体现当时某些变量的变量名、类型、值时十分有用。

It is worth mentioning that the Mole library provides a convenient variable logging function, which is very useful when the variable names, types, and values of certain variables at that time need to be reflected in the log.


## Usage
install Mole library
```shell
git clone https://github.com/OoShawnoO/Mole.git
mkdir build && cd build
cmake ..
make
```
usage for cmake
```cmake
include_directories(Mole)    #("path to Mole.h")
link_directories(Mole/build) #(path to libMole.so/libMole.dll/libMole.a/libMole.lib")
target_link_libraries({TARGET} PRIVATE Mole)
```
usage in code
```c++
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
    MOLE_TRACE(channel_name,"xxx"); /* 2024-04-16 00:18:12 [TRACE] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:56] */
    /* level info */
    MOLE_INFO(channel_name,"xxx");  /*2024-04-16 00:18:12 [INFO] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:58] */
    /* level warn */
    MOLE_WARN(channel_name,"xxx");  /* 2024-04-16 00:18:12 [WARN] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:60] */
    /* level error */
    MOLE_ERROR(channel_name,"xxx"); /* 2024-04-16 00:18:12 [ERROR] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:62] */
    /* level fatal */
    MOLE_FATAL(channel_name,"xxx"); /* 2024-04-16 00:18:12 [FATAL] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:64] */
    
    
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
    /*
    2024-04-16 00:18:12 [INFO] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:74]
    ----------------------------
    int32_t x=1
    float y=3.140000
    double z=1.414000
    const char* cstr=hello world!
    std::string stdstr=hello world!
    std::vector vec=[1,2,3,4,5,]
    std::unordered_map umap={{2:b},{1:a},}
    ---------------------------- 
    */
    
    
    /* MOLE_VAR() macro in order to describle variable,return std::string */
    /* MOLE_VAR() 宏将对变量进行解释,返回值是std::string */
    
    /*
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
     */
    Test t{};
    MOLE_INFO(channel_name,"xxx",{ MOLE_VAR(t)});
    /*
     2024-04-16 00:18:12 [INFO] xxx => [channel_name] [/mnt/d/Projects/C++/Mole/test/main.cpp:84]
        ----------------------------
        Test t=
        int32_t test_object.x=1
        float test_object.y=3.140000
        double test_object.z=1.414000
        ----------------------------
     */
}
```