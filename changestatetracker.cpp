#include "changestatetracker.h"


uint64_t StateChangeTracker::LastChannelsUpdate()
{
    return channelsUpdate.load(std::memory_order_relaxed);
}

uint64_t StateChangeTracker::LastRecordingsUpdate()
{
    return recordingsUpdate.load(std::memory_order_relaxed);
}

uint64_t StateChangeTracker::LastTimersUpdate()
{
    return timersUpdate.load(std::memory_order_relaxed);
}

uint64_t StateChangeTracker::LastEventsUpdate()
{
    return eventsUpdate.load(std::memory_order_relaxed);
}

void StateChangeTracker::Update(std::atomic<uint64_t> &lastChanged)
{
    lastChanged.store(monotonicTimestampNanoSeconds(), std::memory_order_relaxed);
}

void StateChangeTracker::UpdateChannels()
{
    Update(channelsUpdate);
}

void StateChangeTracker::UpdateRecordings()
{
    Update(recordingsUpdate);
}

void StateChangeTracker::UpdateTimers()
{
    Update(timersUpdate);
}

void StateChangeTracker::UpdateEvents()
{
    Update(eventsUpdate);
}
