# Progress
- ☑ 开通菠菜
- ☑ 余额
- ☑ 开箱
- ☑ 开启关闭
- ☑ 帮助
- ☑ !roll
- ☑ 禁烟
- ☐ 天气
- ☐ 吃什么
- ☐ 玩什么
- ☐ 抽卡
- ☐ 摇号
- ☐ 活动开箱
- ☐ 


# Dependencies
- C++17 compatible compiler
- CMake
- libcurl
- Boost (for http)
- sqlite3


# Fetch
git clone https://github.com/yaasdf/bigbedbot-mirai-http
git submodule update --init --recursive


# Build
```shell
mkdir build
cmake -S . -B build/RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/RelWithDebInfo
```

# Install
```shell
cp build/RelWithDebInfo/bbb ~/mirai-console/bbb/
```
