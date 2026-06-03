#include "changestatecounter.h"

static uint64_t g_statusVersion = 0;
static uint64_t g_channelsVersion = 0;
static uint64_t g_recordingsVersion = 0;
static uint64_t g_timersVersion = 0;
static uint64_t g_eventsVersion = 0;

uint64_t ChangeStateCounter::StatusVersion()
{
    return g_statusVersion;
}

uint64_t ChangeStateCounter::ChannelsVersion()
{
    return g_channelsVersion;
}

uint64_t ChangeStateCounter::RecordingsVersion()
{
    return g_recordingsVersion;
}

uint64_t ChangeStateCounter::TimersVersion()
{
    return g_timersVersion;
}

uint64_t ChangeStateCounter::EventsVersion()
{
    return g_eventsVersion;
}

void ChangeStateCounter::IncrementStatus()
{
    ++g_statusVersion;
}

void ChangeStateCounter::IncrementChannels()
{
    ++g_channelsVersion;
}

void ChangeStateCounter::IncrementRecordings()
{
    ++g_recordingsVersion;
}

void ChangeStateCounter::IncrementTimers()
{
    ++g_timersVersion;
}

void ChangeStateCounter::IncrementEvents()
{
    ++g_eventsVersion;
}
