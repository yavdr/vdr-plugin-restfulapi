#ifndef __SCRAPER2VDRSERVICES_H
#define __SCRAPER2VDRSERVICES_H

#include <string>
#include <vector>
#include <vdr/epg.h>
#include <vdr/recording.h>
#include <vdr/plugin.h>
#include "../tools.h"

enum tvType {
    tSeries,
    tMovie,
    tNone,
};

/*********************************************************************
* Helper Structures
*********************************************************************/
class cTvMedia {
public:
	cTvMedia(void) {
		path = "";
		width = height = 0;
	};
    std::string path;
    int width;
    int height;
};

class cEpisode {
public:
    cEpisode(void) {
        number = 0;
        season = 0;
        name = "";
        firstAired = "";
        guestStars = "";
        overview = "";
        rating = 0.0;
    };
    int number;
    int season;
    std::string name;
    std::string firstAired;
    std::string guestStars;
    std::string overview;
    float rating;
    cTvMedia episodeImage;
};

class cActor {
public:
    cActor(void) {
        name = "";
        role = "";
    };
    std::string name;
    std::string role;
    cTvMedia actorThumb;
};

/*********************************************************************
* Data Structures for Service Calls
*********************************************************************/

// Data structure for service "GetEventType"
class ScraperGetEventType {
public:
  ScraperGetEventType(void) {
    event = NULL;
    recording = NULL;
    type = tNone;
    movieId = 0;
    seriesId = 0;
    episodeId = 0;
  };
// in
  const cEvent *event;             // check type for this event
  const cRecording *recording;     // or for this recording
//out
  tvType type;                	 //typeSeries or typeMovie
  int movieId;
  int seriesId;
  int episodeId;
};

//Data structure for full series and episode information
class cMovie {
public:
    cMovie(void) {
        title = "";
        originalTitle = "";
        tagline = "";    
        overview = "";
        adult = false;
        collectionName = "";
        budget = 0;
        revenue = 0;
        genres = "";
        homepage = "";
        releaseDate = "";
        runtime = 0;
        popularity = 0.0;
        voteAverage = 0.0;
    };
//IN
    int movieId;                    // movieId fetched from ScraperGetEventType
//OUT    
    std::string title;
    std::string originalTitle;
    std::string tagline;    
    std::string overview;
    bool adult;
    std::string collectionName;
    int budget;
    int revenue;
    std::string genres;
    std::string homepage;
    std::string releaseDate;
    int runtime;
    float popularity;
    float voteAverage;
    cTvMedia poster;
    cTvMedia fanart;
    cTvMedia collectionPoster;
    cTvMedia collectionFanart;
    std::vector<cActor> actors;
};

//Data structure for full series and episode information
class cSeries {
public:
    cSeries(void) {
        seriesId = 0;
        episodeId = 0;
        name = "";
        overview = "";
        firstAired = "";
        network = "";
        genre = "";
        rating = 0.0;
        status = "";
    };
//IN
    int seriesId;                   // seriesId fetched from ScraperGetEventType
    int episodeId;                  // episodeId fetched from ScraperGetEventType
//OUT
    std::string name;
    std::string overview;
    std::string firstAired;
    std::string network;
    std::string genre;
    float rating;
    std::string status;
    cEpisode episode;
    std::vector<cActor> actors;
    std::vector<cTvMedia> posters;
    std::vector<cTvMedia> banners;
    std::vector<cTvMedia> fanarts;
    cTvMedia seasonPoster;
};

// Data structure for service "GetPosterBanner"
class ScraperGetPosterBanner {
public:
	ScraperGetPosterBanner(void) {
		type = tNone;
	};
// in
    const cEvent *event;             // check type for this event 
//out
    tvType type;                	 //typeSeries or typeMovie
    cTvMedia poster;
    cTvMedia banner;
};

// Data structure for service "GetPoster"
class ScraperGetPoster {
public:
// in
    const cEvent *event;             // check type for this event
    const cRecording *recording;     // or for this recording
//out
    cTvMedia poster;
};

// Data structure for service "GetPosterThumb"
class ScraperGetPosterThumb {
public:
// in
    const cEvent *event;             // check type for this event
    const cRecording *recording;     // or for this recording
//out
    cTvMedia poster;
};



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
  bool getEventType(ScraperGetEventType &eventType);
  void getSeriesMedia(SerAdditionalMedia &am, ScraperGetEventType &eventType);
  void getMovieMedia(SerAdditionalMedia &am, ScraperGetEventType &eventType);
public:
  explicit Scraper2VdrService();
  virtual ~Scraper2VdrService();
  bool getEventMedia(cEvent *event, struct SerAdditionalMedia &am);
  bool getRecordingMedia(cRecording *recording, struct SerAdditionalMedia &am);
};

#endif //__SCRAPER2VDRSERVICES_H
