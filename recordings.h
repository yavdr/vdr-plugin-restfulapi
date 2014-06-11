#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "tools.h"
#include "services/scraper2vdr.h"

#include <vdr/cutter.h>
#include <vdr/recording.h>
#include <vdr/videodir.h>

class RecordingsResponder : public cxxtools::http::Responder
{
  public:
    explicit RecordingsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }

    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void deleteRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void deleteRecordingByName(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void showRecordings(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void saveMarks(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void deleteMarks(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void cutRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void showCutterStatus(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void playRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void rewindRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void moveRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyRecordingMoved(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, cRecording* recording);
};

typedef cxxtools::http::CachedService<RecordingsResponder> RecordingsService;

class SerMarks
{
  public:
    std::vector< std::string > marks;
};

struct SerRecording
{
  int Number;
  cxxtools::String Name;
  cxxtools::String FileName;
  cxxtools::String RelativeFileName;
  cxxtools::String ChannelID;
  bool IsNew;
  bool IsEdited;
  bool IsPesRecording;
  int Duration;
  int FileSizeMB;
  double FramesPerSecond;
  SerMarks Marks;
  cxxtools::String EventTitle;
  cxxtools::String EventShortText;
  cxxtools::String EventDescription;
  int EventStartTime;
  int EventDuration;
  cxxtools::String Scraper;
  cxxtools::String ScraperPoster;
  cxxtools::String ScraperFanart;
  cxxtools::String ScraperBanner;
  cxxtools::String ScraperActors;
  cxxtools::String ScraperCollectionPoster;
  cxxtools::String ScraperCollectionFanart;
  int SeriesId;
  cxxtools::String SeriesName;
  cxxtools::String SeriesOverview;
  cxxtools::String SeriesFirstAired;
  cxxtools::String SeriesNetwork;
  cxxtools::String SeriesGenre;
  double SeriesRating;
  cxxtools::String SeriesStatus;
  int EpisodeId;
  int EpisodeNumber;
  int EpisodeSeason;
  cxxtools::String EpisodeName;
  cxxtools::String EpisodeFirstAired;
  cxxtools::String EpisodeGuestStars;
  cxxtools::String EpisodeOverview;
  double EpisodeRating;
  cxxtools::String EpisodeImage;
  int MovieId;
  cxxtools::String MovieTitle;
  cxxtools::String MovieOriginalTitle;
  cxxtools::String MovieTagline;
  cxxtools::String MovieOverview;
  bool MovieAdult;
  cxxtools::String MovieCollectionName;
  int MovieBudget;
  int MovieRevenue;
  cxxtools::String MovieGenres;
  cxxtools::String MovieHomepage;
  cxxtools::String MovieReleaseDate;
  int MovieRuntime;
  double MoviePopularity;
  double MovieVoteAverage;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p);

class RecordingList : public BaseList
{
  protected:
    bool read_marks;
    int total;
    StreamExtension *s;
  public:
    RecordingList(std::ostream* _out, bool _read_marks);
    virtual ~RecordingList();
    virtual void init() { };
    virtual void addRecording(cRecording* recording, int nr) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlRecordingList : RecordingList
{
  public:
    HtmlRecordingList(std::ostream* _out, bool _read_marks) : RecordingList(_out, _read_marks) { };
    ~HtmlRecordingList() { };
    virtual void init();
    virtual void addRecording(cRecording* recording, int nr);
    virtual void finish();
};

class JsonRecordingList : RecordingList
{
  private:
    std::vector < struct SerRecording > serRecordings;
  public:
    JsonRecordingList(std::ostream* _out, bool _read_marks) : RecordingList(_out, _read_marks) { };
    ~JsonRecordingList() { };
    virtual void addRecording(cRecording* recording, int nr);
    virtual void finish();
};

class XmlRecordingList : RecordingList
{
  public:
    XmlRecordingList(std::ostream* _out, bool _read_marks) : RecordingList(_out, _read_marks) { };
    ~XmlRecordingList() { };
    virtual void init();
    virtual void addRecording(cRecording* recording, int nr);
    virtual void finish();
};
