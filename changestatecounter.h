#ifndef __CHANGESTATECOUNTER_H
#define __CHANGESTATECOUNTER_H

#include <atomic>
#include <stdint.h>

class ChangeStateCounter
{
public:
    static uint64_t InstanceStarted();
    static uint64_t LastChange();

    static uint64_t StatusVersion();
    static uint64_t ChannelsVersion();
    static uint64_t RecordingsVersion();
    static uint64_t TimersVersion();
    static uint64_t EventsVersion();

    static void IncrementStatus();
    static void IncrementChannels();
    static void IncrementRecordings();
    static void IncrementTimers();
    static void IncrementEvents();

private:
    static void MarkChanged(std::atomic<uint64_t>& counter);

    static const uint64_t instanceStarted;
    static std::atomic<uint64_t> lastChange;
    static std::atomic<uint64_t> statusVersion;
    static std::atomic<uint64_t> channelsVersion;
    static std::atomic<uint64_t> recordingsVersion;
    static std::atomic<uint64_t> timersVersion;
    static std::atomic<uint64_t> eventsVersion;
};

#endif
