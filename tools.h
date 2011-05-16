#include <vdr/channels.h>
#include <unistd.h>
#include <string>

int GetIntParam(std::string qparams, std::string param);
cChannel* GetChannel(int number);
