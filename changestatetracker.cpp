#include "changestatetracker.h"

#include <chrono>

namespace {
uint64_t monotonicTimestampNanoSeconds() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()).count();
}


}

uint64_t StateChangeTracker::lastChannelsUpdate()
{
    return channelsUpdate.load(std::memory_order_relaxed);
}

uint64_t StateChangeTracker::lastRecordingsUpdate()
{
    return recordingsUpdate.load(std::memory_order_relaxed);
}

uint64_t StateChangeTracker::lastTimersUpdate()
{
    return timersUpdate.load(std::memory_order_relaxed);
}

uint64_t StateChangeTracker::lastEventsUpdate()
{
    return eventsUpdate.load(std::memory_order_relaxed);
}

void StateChangeTracker::update(std::atomic<uint64_t>& lastChanged)
{
    lastChanged.store(monotonicTimestampNanoSeconds(), std::memory_order_relaxed);
}

void StateChangeTracker::updateChannels()
{
    update(channelsUpdate);
}

void StateChangeTracker::updateRecordings()
{
    update(recordingsUpdate);
}

void StateChangeTracker::updateTimers()
{
    update(timersUpdate);
}

void StateChangeTracker::updateEvents()
{
    update(eventsUpdate);
}

