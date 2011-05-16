#include <cxxtools/serializationinfo.h>

struct SerTimer
{
  int Start;
  int Stop;
  int Priority;
  int Lifetime;
  int EventID;
  int WeekDays;
  int Day;
  int Channel;
  bool IsRecording;
  bool IsPending;
  cxxtools::String FileName;
};

struct SerTimers
{
  std::vector < struct SerTimer > timer;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerTimer& t);
void operator>>= (const cxxtools::SerializationInfo& si, SerTimer& t);
void operator<<= (cxxtools::SerializationInfo& si, const SerTimers& t);

struct SerEvent
{
  int Id;
  cxxtools::String Title;
  cxxtools::String ShortText;
  cxxtools::String Description;
  int StartTime;
  int Duration;
};

struct SerEvents
{
  std::vector < struct SerEvent > event;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e);
void operator<<= (cxxtools::SerializationInfo& si, const SerEvents& e);

struct SerChannel
{
  cxxtools::String Name;
  int Number;
  int Transponder;
  cxxtools::String Stream;
  bool IsAtsc;
  bool IsCable;
  bool IsTerr;
  bool IsSat;
};

struct SerChannels
{
  std::vector < struct SerChannel > channel;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerChannel& c);
void operator<<= (cxxtools::SerializationInfo& si, const SerChannels& c);

struct SerRecording
{
  cxxtools::String Name;
  cxxtools::String FileName;
  bool IsNew;
  bool IsEdited;
  bool IsPesRecording;
};

struct SerRecordings
{
  std::vector < struct SerRecording > recording;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p);
void operator<<= (cxxtools::SerializationInfo& si, const SerRecordings& p);

