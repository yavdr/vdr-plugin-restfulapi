#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <locale.h>
#include <time.h>
#ifdef USE_LIBMAGICKPLUSPLUS
#include <Magick++.h>
#endif
#include "scraper2vdr/services.h"
#include "tools.h"

#ifndef SCRAPER2VDRESTFULAPI_H
#define SCRAPER2VDRESTFULAPI_H

#ifdef USE_LIBMAGICKPLUSPLUS
using namespace Magick;
#endif

struct SerActor
{
  cxxtools::String Name;
  cxxtools::String Role;
  cxxtools::String Thumb;
};

struct SerImage
{
  cxxtools::String Path;
  int Width;
  int Height;
};

struct SerAdditionalMedia
{
  cxxtools::String Scraper;
  int SeriesId;
  cxxtools::String SeriesName;
  cxxtools::String SeriesOverview;
  cxxtools::String SeriesFirstAired;
  cxxtools::String SeriesNetwork;
  cxxtools::String SeriesGenre;
  float SeriesRating;
  cxxtools::String SeriesStatus;
  int EpisodeId;
  int EpisodeNumber;
  int EpisodeSeason;
  cxxtools::String EpisodeName;
  cxxtools::String EpisodeFirstAired;
  cxxtools::String EpisodeGuestStars;
  cxxtools::String EpisodeOverview;
  float EpisodeRating;
  cxxtools::String EpisodeImage;
  std::vector< struct SerImage > Posters;
  std::vector< struct SerImage > Banners;
  std::vector< struct SerImage > Fanarts;
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
  float MoviePopularity;
  float MovieVoteAverage;
  cxxtools::String MoviePoster;
  cxxtools::String MovieFanart;
  cxxtools::String MovieCollectionPoster;
  cxxtools::String MovieCollectionFanart;
  std::vector< struct SerActor > Actors;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerActor& a);
void operator<<= (cxxtools::SerializationInfo& si, const SerImage& i);
void operator<<= (cxxtools::SerializationInfo& si, const SerAdditionalMedia& am);


class Scraper2VdrService {
private:
  cPlugin *getScraperPlugin(void);
  cPlugin *scraper;
  std::string epgImagesDir;
  bool getEventType		(ScraperGetEventType &eventType);
  void getSeriesMedia		(SerAdditionalMedia &am, ScraperGetEventType &eventType);
  void getMovieMedia		(SerAdditionalMedia &am, ScraperGetEventType &eventType);
  void getSeriesMedia		(StreamExtension* s, ScraperGetEventType &eventType);
  void getMovieMedia		(StreamExtension* s, ScraperGetEventType &eventType);
  std::string getSeriesMedia	(ScraperGetEventType &eventType);
  std::string getMovieMedia	(ScraperGetEventType &eventType);
  bool getMedia			(ScraperGetEventType &eventType, SerAdditionalMedia &am);
  bool getMedia			(ScraperGetEventType &eventType, StreamExtension* s);
  std::string getMedia		(ScraperGetEventType &eventType);
  std::string cleanImagePath(std::string path);
public:
  explicit Scraper2VdrService();
  virtual ~Scraper2VdrService();
  bool getMedia(const cEvent *event, SerAdditionalMedia &am);
  bool getMedia(const cRecording *recording, SerAdditionalMedia &am);
  bool getMedia(const cEvent *event, StreamExtension *s);
  std::string getMedia(const cRecording *recording);
};

class ScraperImageResponder : public cxxtools::http::Responder {
private:
#ifdef USE_LIBMAGICKPLUSPLUS
  std::string parseResize(std::string url, int& width, int& height, bool& aspect);
  bool resizeImage(std::string source, std::string target, int& width, int& height, bool& aspect);
#endif
  bool hasPath(std::string targetPath);
public:
  explicit ScraperImageResponder(cxxtools::http::Service& service) : cxxtools::http::Responder(service) {}
  virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<ScraperImageResponder> ScraperService;

#endif // SCRAPER2VDRESTFULAPI_H
