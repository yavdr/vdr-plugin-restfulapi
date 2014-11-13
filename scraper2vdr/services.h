#ifndef __SCRAPER2VDRSERVICES_H
#define __SCRAPER2VDRSERVICES_H

#include <string>
#include <vector>
#include <vdr/epg.h>
#include <vdr/recording.h>
#include <vdr/plugin.h>

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

#endif //__SCRAPER2VDRSERVICES_H
