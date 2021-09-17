# Progress
- ☑ 开通菠菜
- ☑ 余额
- ☑ 开箱
- ☑ 开启关闭
- ☑ 帮助
- ☑ !roll
- ☑ 禁烟
- ☑ 天气
- ☑ 吃什么
- ☑ 玩什么
- ☑ 抽卡
- ~~☑ 摇号~~
- ☐ 活动开箱
- ☐ 


# Dependencies
- C++17 compatible compiler (tested on g++ 10.2.0)
- CMake 3.8
- libcurl
- Boost (for http)
- sqlite3

Installing dependencies on Windows
```ps
vcpkg install curl:x64-windows
vcpkg install boost:x64-windows-static
vcpkg install sqlite3:x64-windows
```


# Fetch
```shell
git clone https://github.com/yaasdf/bigbedbot-mirai-http
git submodule update --init --recursive
```

# Build
Linux
```shell
mkdir build
cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

Now support debugging with console
```shell
mkdir build
cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```

# Install
```shell
mkdir -p ~/mirai-console/bbb/config
cp default/* ~/mirai-console/bbb/config
cp build/RelWithDebInfo/bbb ~/mirai-console/bbb/
```
