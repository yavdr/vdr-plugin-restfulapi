#ifndef __CHANGESTATECOUNTER_H
#define __CHANGESTATECOUNTER_H

#include <algorithm>
#include <atomic>
#include <fstream>
#include <stdint.h>

namespace {
    inline std::string readBootID() {
        // return the boot id with dashes removed
        std::ifstream file("/proc/sys/kernel/random/boot_id");
        std::string id;
        if (file && std::getline(file, id)) {
            id.erase(std::remove(id.begin(), id.end(), '-'), id.end());
            return id;
        }
        return "00000000000000000000000000000000"; // Fallback
    }
}

class StateChangeTracker
{
public:

    inline static const std::string bootID = readBootID();
    static uint64_t lastChannelsUpdate();
    static uint64_t lastRecordingsUpdate();
    static uint64_t lastTimersUpdate();
    static uint64_t lastEventsUpdate();

    static void updateChannels();
    static void updateRecordings();
    static void updateTimers();
    static void updateEvents();

private:
    static void update(std::atomic<uint64_t>& counter);
    inline static std::atomic<uint64_t> channelsUpdate{0};
    inline static std::atomic<uint64_t> recordingsUpdate{0};
    inline static std::atomic<uint64_t> timersUpdate{0};
    inline static std::atomic<uint64_t> eventsUpdate{0};

};

#endif
