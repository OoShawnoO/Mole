# Mole
## 介绍 Introduce
Mole是一个异步日志库，提供日志频道记录日志的功能。

Mole is an asynchronous logging library that provides log channel recording functionality.

日志频道是日志控制主体，不同的日志频道可以设置不同的日志过滤等级、日志保存与否、日志是否输出在终端等，日志的输出文件路径也不同。

The log channel is the subject of log control. Different log channels can set different log filtering levels, whether to save logs, whether to output logs to the terminal, etc. The output file paths of logs are also different.

值得一提的是Mole库提供了便捷的变量记录功能，当需要在日志中体现当时某些变量的变量名、类型、值时十分有用。

It is worth mentioning that the Mole library provides a convenient variable logging function, which is very useful when the variable names, types, and values of certain variables at that time need to be reflected in the log.

## 基准测试 & Benchmark
对比Mole与spdlog库，虽然Mole库功能没有spdlog丰富，但是日志库的主要功能在于日志记录与回显。

两者均不进行日志回显，只输出日志到日志文件，测试1000条日志记录
```text
Benchmark Software:Google Benchmark
Test Environment: 
    System: Windows 11 23H2 => WSL Ubuntu 22.04
    Compiler: GNU gcc 11.4.0
    CPU: Intel(R) Core(TM) i7-10875H@2.30GHz
    Memory: 16G * 2 (2304.01 MHz)
```
### 测试结果 & Test Result
```text
Run on (16 X 2304.01 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 256 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 0.10, 0.07, 0.02
------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations
------------------------------------------------------------------------
BM_CXX_MOLE/iterations:1000         2354 ns         1796 ns         1000
BM_CXX_SPDLOG/iterations:1000       4514 ns          876 ns         1000
```
### 测试代码 & Test Code
```c++
#include <benchmark/benchmark.h>
#include <Mole/Mole.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

static void BM_CXX_MOLE(benchmark::State& state) {
    remove("log/hzd.log");
    MOLE_CHANNEL_SHOWLOG("hzd",false);
    MOLE_CHANNEL_SAVEABLE("hzd",true);
    const char* str = "11111111111";
    int x = 2;
    for(auto _ : state) {
        MOLE_INFO("hzd","~",{MOLE_VAR(x),MOLE_VAR(str)});
    }
}

static void BM_CXX_SPDLOG(benchmark::State& state) {
    remove("log/spdlog.log");
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S [%l] %v");
    const char* str = "11111111111";
    int x = 2;
    auto flogger = spdlog::basic_logger_mt("file_logger","log/spdlog.log");
    for(auto _ : state ) {
        flogger->info("~\n----------------\n{} {} {}\n{} {} {}\n------------------",typeid(x).name(),"x",x,typeid(str).name(),"str",str);
    }
}
BENCHMARK(BM_CXX_MOLE)->Iterations(1000);
BENCHMARK(BM_CXX_SPDLOG)->Iterations(1000);
BENCHMARK_MAIN();
```

## 使用说明 & Usage
默认安装Mole & default install Mole
```shell
git clone https://github.com/OoShawnoO/Mole.git
cd Mole
mkdir build && cd build
cmake ..
# install for system
sudo make install
```
自定义安装路径 & custom install path
```shell
git clone https://github.com/OoShawnoO/Mole.git
cd Mole
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="path to install" ..
sudo make install
```
卸载Mole & uninstall Mole
```shell
# uninstall
sudo make uninstall
```

usage for cmake
```cmake
# 使用make install时 & when use make install
# ---------------------------------------------
find_package(Mole REQUIRED)
target_link_libraries({TARGET} PRIVATE Mole::Mole)
# ---------------------------------------------

# 仅自定义安装需要设置 & just custom install need
# ---------------------------------------------
include_directories(path to Mole.h)
link_directories(path to libMole.so/libMole.dll)

target_link_libraries({TARGET} PRIVATE Mole)
# ---------------------------------------------
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

Test() = default;
Test(int _x,float _y,double _z) : x(_x),y(_y),z(_z) {}
};
MOLE_DEFINE(Test,test_object) {
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
MOLE_CHANNEL_SAVEABLE("channel_name",true);

/* set log channel whether output log to terminal,default true */
/* 设置日志频道是否输出日志到终端,默认开启 */
MOLE_CHANNEL_SHOWLOG("channel_name",true);

/* set log channel filter level */
/* 设置日志频道过滤等级 */
/* low to high TRACE -> INFO -> WARN -> ERROR -> FATAL */
/* 由低到高 TRACE -> INFO -> WARN -> ERROR -> FATAL */
MOLE_CHANNEL_LEVEL("channel_name","TRACE");

/* log */
/* 打日志 */
/* level trace */
MOLE_TRACE("channel_name","xxx");
/* level info */
MOLE_INFO("channel_name","xxx");
/* level warn */
MOLE_WARN("channel_name","xxx");
/* level error */
MOLE_ERROR("channel_name","xxx");
/* level fatal */
MOLE_FATAL("channel_name","xxx");

int x = 1;
float y = 3.14;
double z = 1.414;
const char* cstr = "hello world!";
std::string stdstr = "hello world!";
std::vector<int> vec = {1,2,3,4,5};
std::unordered_map<int,std::string> umap  = { {1,"a"},{2,"b"} };
/* 日志记录变量 */
MOLE_INFO("channel_name","xxx",{
MOLE_VAR(x),MOLE_VAR(y),MOLE_VAR(z),MOLE_VAR(str),MOLE_VAR(cstr),
MOLE_VAR(stdstr),MOLE_VAR(vec),MOLE_VAR(umap)
});

/* MOLE_VAR() macro in order to describle variable,return std::string */
/* MOLE_VAR() 宏将对变量进行解释,返回值是std::string */

Test t{};

MOLE_INFO("channel_name","xxx",{ MOLE_VAR(t)});

const Test ct{1,2.0,3.0};

MOLE_INFO("channel_name","xxx",{ MOLE_VAR(ct) });
}
```