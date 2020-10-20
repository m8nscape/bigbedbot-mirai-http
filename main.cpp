#include <thread>
#include <chrono>
#include <csignal>

#include "core.h"

void hs_end(int signal)
{
    core::shutdown();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char** argv)
{

    if (int ret = core::initialize()) return ret;
    if (int ret = core::init_app_and_start()) return ret;

    std::signal(SIGINT, hs_end);
    std::signal(SIGTERM, hs_end);

    while (core::isBotStarted())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}