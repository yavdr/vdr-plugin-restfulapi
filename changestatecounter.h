#ifndef __CHANGESTATECOUNTER_H
#define __CHANGESTATECOUNTER_H

#include <stdint.h>

class ChangeStateCounter
{
public:
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
};

#endif
