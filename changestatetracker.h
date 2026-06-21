#ifndef __CHANGESTATECOUNTER_H
#define __CHANGESTATECOUNTER_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <random>
#include <iostream>
#include <stdint.h>

namespace
{
    inline uint64_t monotonicTimestampNanoSeconds()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    inline std::string GenerateRandomHexString(size_t length)
    {
        const char hex_chars[] = "0123456789abcdef";

        std::random_device rd;
        std::mt19937 generator(rd());

        std::uniform_int_distribution<> distribution(0, 15);

        std::string hex_string;
        hex_string.reserve(length);

        for (size_t i = 0; i < length; ++i)
        {
            hex_string += hex_chars[distribution(generator)];
        }

        return hex_string;
    }

    inline std::string ReadBootID()
    {
        std::ifstream file("/proc/sys/kernel/random/boot_id");
        std::string id;
        if (file && std::getline(file, id))
        {
            // return the boot id with dashes removed
            id.erase(std::remove(id.begin(), id.end(), '-'), id.end());
            return id;
        }
        return GenerateRandomHexString(32); // Fallback
    }
}

class StateChangeTracker
{
public:
    inline static const std::string bootID = ReadBootID();
    static uint64_t LastChannelsUpdate();
    static uint64_t LastRecordingsUpdate();
    static uint64_t LastTimersUpdate();
    static uint64_t LastEventsUpdate();

    static void UpdateChannels();
    static void UpdateRecordings();
    static void UpdateTimers();
    static void UpdateEvents();

private:
    static void Update(std::atomic<uint64_t> &counter);
    inline static std::atomic<uint64_t> channelsUpdate{monotonicTimestampNanoSeconds()};
    inline static std::atomic<uint64_t> recordingsUpdate{monotonicTimestampNanoSeconds()};
    inline static std::atomic<uint64_t> timersUpdate{monotonicTimestampNanoSeconds()};
    inline static std::atomic<uint64_t> eventsUpdate{monotonicTimestampNanoSeconds()};
};

#endif
