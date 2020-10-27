#include <thread>
#include <memory>
#include <chrono>
#include <ctime>

#include "time_evt.h"

std::vector<std::function<void()>> timedEventQueue[24][60];
std::vector<std::function<void()>> timedEventQueueEveryMin;

bool timedThreadRunning = false;
void timedEventLoop()
{
	using namespace std::chrono_literals;
	int prev_hr = 0, prev_min = 0;
	while (timedThreadRunning)
	{
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);
		/*
		if (prev_min == tm.tm_min)
		{
			std::this_thread::sleep_for(30s);
			continue;
		}
		*/

		for (auto& f : timedEventQueue[tm.tm_hour][tm.tm_min])
		{
			if (f) f();
		}

		for (auto& f : timedEventQueueEveryMin)
		{
			if (f) f();
		}

		prev_hr = tm.tm_hour;
		prev_min = tm.tm_min;
		std::this_thread::sleep_for(1min);
	}
}

std::shared_ptr<std::thread> timedEventThread = nullptr;

void startTimedEvent()
{
	timedThreadRunning = true;
	timedEventThread = std::make_shared<std::thread>(timedEventLoop);
	timedEventThread->detach();
}

void stopTimedEvent()
{
	timedThreadRunning = false;
}

void addTimedEventEveryMin(std::function<void()> f)
{
	timedEventQueueEveryMin.push_back(f);
}

void addTimedEvent(std::function<void()> f, int hour, int min)
{
	if (hour < 0 || hour > 23) return;
	if (min < 0  || min > 59)  return;
	timedEventQueue[hour][min].push_back(f);
}

void clearTimedEvent()
{
	for (int h = 0; h < 24; ++h)
		for (int m = 0; m < 60; ++m)
		{
			timedEventQueue[h][m].clear();
		}
	timedEventQueueEveryMin.clear();
}