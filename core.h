#pragma once

//extern time_t banTime_me;

namespace core
{
// core.cpp
bool isBotStarted();
int initialize();
int config();
int init_app_and_start();
int shutdown();

// modules.cpp
void init_modules();
void shutdown_modules();
void add_timed_events();
void add_msg_callbacks();
}