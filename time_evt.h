#pragma once
#include <functional>

void startTimedEvent();
void stopTimedEvent();

void addTimedEvent(std::function<void()> f, int hour, int min);

void clearTimedEvent();