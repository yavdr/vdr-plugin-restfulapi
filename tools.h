#include <vdr/channels.h>
#include <unistd.h>
#include <string>
#include <vector>

// Request Helper Methods
int GetIntParam(std::string qparams, int level);
bool IsFormat(std::string qparams, std::string format);

// VDR API Helper Methods
cChannel* GetChannel(int number);

