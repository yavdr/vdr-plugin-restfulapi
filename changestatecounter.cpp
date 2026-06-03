#include "changestatecounter.h"

#include <ctime>

namespace {
uint64_t CurrentUnixTime()
{
    return static_cast<uint64_t>(std::time(NULL));
}
}

const uint64_t ChangeStateCounter::instanceStarted = CurrentUnixTime();
std::atomic<uint64_t> ChangeStateCounter::lastChange(ChangeStateCounter::instanceStarted);
std::atomic<uint64_t> ChangeStateCounter::statusVersion(0);
std::atomic<uint64_t> ChangeStateCounter::channelsVersion(0);
std::atomic<uint64_t> ChangeStateCounter::recordingsVersion(0);
std::atomic<uint64_t> ChangeStateCounter::timersVersion(0);
std::atomic<uint64_t> ChangeStateCounter::eventsVersion(0);

uint64_t ChangeStateCounter::InstanceStarted()
{
    return instanceStarted;
}

uint64_t ChangeStateCounter::LastChange()
{
    return lastChange.load(std::memory_order_relaxed);
}

uint64_t ChangeStateCounter::StatusVersion()
{
    return statusVersion.load(std::memory_order_relaxed);
}

uint64_t ChangeStateCounter::ChannelsVersion()
{
    return channelsVersion.load(std::memory_order_relaxed);
}

uint64_t ChangeStateCounter::RecordingsVersion()
{
    return recordingsVersion.load(std::memory_order_relaxed);
}

uint64_t ChangeStateCounter::TimersVersion()
{
    return timersVersion.load(std::memory_order_relaxed);
}

uint64_t ChangeStateCounter::EventsVersion()
{
    return eventsVersion.load(std::memory_order_relaxed);
}

void ChangeStateCounter::IncrementStatus()
{
    MarkChanged(statusVersion);
}

void ChangeStateCounter::IncrementChannels()
{
    MarkChanged(channelsVersion);
}

void ChangeStateCounter::IncrementRecordings()
{
    MarkChanged(recordingsVersion);
}

void ChangeStateCounter::IncrementTimers()
{
    MarkChanged(timersVersion);
}

void ChangeStateCounter::IncrementEvents()
{
    MarkChanged(eventsVersion);
}

void ChangeStateCounter::MarkChanged(std::atomic<uint64_t>& counter)
{
    counter.fetch_add(1, std::memory_order_relaxed);
    lastChange.store(CurrentUnixTime(), std::memory_order_relaxed);
}
