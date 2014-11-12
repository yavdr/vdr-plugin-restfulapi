#include "scraper2vdr.h"
using namespace std;

Scraper2VdrService::Scraper2VdrService() {
  scraper = getScraperPlugin();
};

Scraper2VdrService::~Scraper2VdrService() {
  scraper = NULL;
};

/**
 * retrieve scraper plugin
 */
cPlugin *Scraper2VdrService::getScraperPlugin () {

  cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
  if ( !pScraper )
    pScraper = cPluginManager::GetPlugin("tvscraper");

  return pScraper;
};

/**
 * determine eventType
 */
bool Scraper2VdrService::getEventType(ScraperGetEventType &eventType) {

  if (scraper) {
    return scraper->Service("GetEventType", &eventType);
  }

  return false;
};

/**
 * retrieve additional media according to event
 */
bool Scraper2VdrService::getEventMedia(cEvent *event, struct SerAdditionalMedia &am) {

  ScraperGetEventType eventType;
  eventType.event = event;

  if (getEventType(eventType)) {
      if (eventType.seriesId > 0) {
	  getSeriesMedia(am, eventType);
	  if (am.SeriesId) {
	      return true;
	  }
      } else if (eventType.movieId > 0) {
	  getMovieMedia(am, eventType);
	  if (am.MovieId) {
	      return true;
	  }
      }
  }

  return false;
};

/**
 * retrieve additional media according to recording
 */
bool Scraper2VdrService::getRecordingMedia(cRecording* recording, struct SerAdditionalMedia &am) {

  ScraperGetEventType eventType;
  eventType.recording = recording;

  if (getEventType(eventType)) {
      struct SerAdditionalMedia am;
      if (eventType.seriesId > 0) {
	  getSeriesMedia(am, eventType);
	  if (am.SeriesId) {
	      return true;
	  }
      } else if (eventType.movieId > 0) {
	  getMovieMedia(am, eventType);
	  if (am.MovieId) {
	      return true;
	  }
      }
  }

  return false;
};

/**
 * enrich additional media structure with series data
 */
void Scraper2VdrService::getSeriesMedia(SerAdditionalMedia &am, ScraperGetEventType &eventType) {

  cSeries series;
  series.seriesId = eventType.seriesId;
  series.episodeId = eventType.episodeId;

  if (!scraper) return;

  if (scraper->Service("GetSeries", &series)) {
    am.MovieId = 0;
    am.Scraper = "series";
    am.SeriesId = series.seriesId;
    am.EpisodeId = series.episodeId;
    am.SeriesName = StringExtension::UTF8Decode(series.name);
    am.SeriesOverview = StringExtension::UTF8Decode(series.overview);
    am.SeriesFirstAired = StringExtension::UTF8Decode(series.firstAired);
    am.SeriesNetwork = StringExtension::UTF8Decode(series.network);
    am.SeriesGenre = StringExtension::UTF8Decode(series.genre);
    am.SeriesRating = series.rating;
    am.SeriesStatus = StringExtension::UTF8Decode(series.status);

    am.EpisodeNumber = series.episode.number;
    am.EpisodeSeason = series.episode.season;
    am.EpisodeName = StringExtension::UTF8Decode(series.episode.name);
    am.EpisodeFirstAired = StringExtension::UTF8Decode(series.episode.firstAired);
    am.EpisodeGuestStars = StringExtension::UTF8Decode(series.episode.guestStars);
    am.EpisodeOverview = StringExtension::UTF8Decode(series.episode.overview);
    am.EpisodeRating = series.episode.rating;
    am.EpisodeImage = StringExtension::UTF8Decode(series.episode.episodeImage.path);

    if (series.actors.size() > 0) {
       int _actors = series.actors.size();
       for (int i = 0; i < _actors; i++) {
	   struct SerActor actor;
	   actor.Name = StringExtension::UTF8Decode(series.actors[i].name);
	   actor.Role = StringExtension::UTF8Decode(series.actors[i].role);
	   actor.Thumb = StringExtension::UTF8Decode(series.actors[i].actorThumb.path);
	   am.Actors.push_back(actor);
       }
    }

    if (series.posters.size() > 0) {
       int _posters = series.posters.size();
       for (int i = 0; i < _posters; i++) {
	   if ((series.posters[i].width > 0) && (series.posters[i].height > 0)) {
	      struct SerImage poster;
	      poster.Path = StringExtension::UTF8Decode(series.posters[i].path);
	      poster.Width = series.posters[i].width;
	      poster.Height = series.posters[i].height;
	      am.Posters.push_back(poster);
	   }
       }
    }

    if (series.banners.size() > 0) {
       int _banners = series.banners.size();
       for (int i = 0; i < _banners; i++) {
	   if ((series.banners[i].width > 0) && (series.banners[i].height > 0)) {
	      struct SerImage banner;
	      banner.Path = StringExtension::UTF8Decode(series.banners[i].path);
	      banner.Width = series.banners[i].width;
	      banner.Height = series.banners[i].height;
	      am.Banners.push_back(banner);
	   }
       }
    }

    if (series.fanarts.size() > 0) {
       int _fanarts = series.fanarts.size();
       for (int i = 0; i < _fanarts; i++) {
	   if ((series.fanarts[i].width > 0) && (series.fanarts[i].height > 0)) {
	      struct SerImage fanart;
	      fanart.Path = StringExtension::UTF8Decode(series.fanarts[i].path);
	      fanart.Width = series.fanarts[i].width;
	      fanart.Height = series.fanarts[i].height;
	      am.Fanarts.push_back(fanart);
	   }
       }
    }
  }
};

/**
 * enrich additional media structure with movie data
 */
void Scraper2VdrService::getMovieMedia(SerAdditionalMedia &am, ScraperGetEventType &eventType) {

  cMovie movie;
  movie.movieId = eventType.movieId;

  if (!scraper) return;

  if (scraper->Service("GetMovie", &movie)) {
      am.SeriesId = 0;
      am.Scraper = "movie";
      am.MovieId = movie.movieId;
      am.MovieTitle = StringExtension::UTF8Decode(movie.title);
      am.MovieOriginalTitle = StringExtension::UTF8Decode(movie.originalTitle);
      am.MovieTagline = StringExtension::UTF8Decode(movie.tagline);
      am.MovieOverview = StringExtension::UTF8Decode(movie.overview);
      am.MovieAdult = movie.adult;
      am.MovieCollectionName = StringExtension::UTF8Decode(movie.collectionName);
      am.MovieBudget = movie.budget;
      am.MovieRevenue = movie.revenue;
      am.MovieGenres = StringExtension::UTF8Decode(movie.genres);
      am.MovieHomepage = StringExtension::UTF8Decode(movie.homepage);
      am.MovieReleaseDate = StringExtension::UTF8Decode(movie.releaseDate);
      am.MovieRuntime = movie.runtime;
      am.MoviePopularity = movie.popularity;
      am.MovieVoteAverage = movie.voteAverage;
      am.MoviePoster = StringExtension::UTF8Decode(movie.poster.path);
      am.MovieFanart = StringExtension::UTF8Decode(movie.fanart.path);
      am.MovieCollectionPoster = StringExtension::UTF8Decode(movie.collectionPoster.path);
      am.MovieCollectionFanart = StringExtension::UTF8Decode(movie.collectionFanart.path);
      if (movie.actors.size() > 0) {
         int _actors = movie.actors.size();
         for (int i = 0; i < _actors; i++) {
             struct SerActor actor;
             actor.Name = StringExtension::UTF8Decode(movie.actors[i].name);
             actor.Role = StringExtension::UTF8Decode(movie.actors[i].role);
             actor.Thumb = StringExtension::UTF8Decode(movie.actors[i].actorThumb.path);
             am.Actors.push_back(actor);
         }
      }
  }
};



void operator<<= (cxxtools::SerializationInfo& si, const SerImage& i)
{
  si.addMember("path") <<= i.Path;
  si.addMember("width") <<= i.Width;
  si.addMember("height") <<= i.Height;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerActor& a)
{
  si.addMember("name") <<= a.Name;
  si.addMember("role") <<= a.Role;
  si.addMember("thumb") <<= a.Thumb;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerAdditionalMedia& am)
{
  if (am.Scraper.length() > 0) {
     si.addMember("type") <<= am.Scraper;
     if (am.SeriesId > 0) {
        si.addMember("series_id") <<= am.SeriesId;
        si.addMember("episode_id") <<= am.EpisodeId;
        si.addMember("name") <<= am.SeriesName;
        si.addMember("overview") <<= am.SeriesOverview;
        si.addMember("first_aired") <<= am.SeriesFirstAired;
        si.addMember("network") <<= am.SeriesNetwork;
        si.addMember("genre") <<= am.SeriesGenre;
        si.addMember("rating") <<= am.SeriesRating;
        si.addMember("status") <<= am.SeriesStatus;

        si.addMember("episode_number") <<= am.EpisodeNumber;
        si.addMember("episode_season") <<= am.EpisodeSeason;
        si.addMember("episode_name") <<= am.EpisodeName;
        si.addMember("episode_first_aired") <<= am.EpisodeFirstAired;
        si.addMember("episode_guest_stars") <<= am.EpisodeGuestStars;
        si.addMember("episode_overview") <<= am.EpisodeOverview;
        si.addMember("episode_rating") <<= am.EpisodeRating;
        si.addMember("episode_image") <<= am.EpisodeImage;
        si.addMember("posters") <<= am.Posters;
        si.addMember("banners") <<= am.Banners;
        si.addMember("fanarts") <<= am.Fanarts;
     }
     else if (am.MovieId > 0) {
        si.addMember("movie_id") <<= am.MovieId;
        si.addMember("title") <<= am.MovieTitle;
        si.addMember("original_title") <<= am.MovieOriginalTitle;
        si.addMember("tagline") <<= am.MovieTagline;
        si.addMember("overview") <<= am.MovieOverview;
        si.addMember("adult") <<= am.MovieAdult;
        si.addMember("collection_name") <<= am.MovieCollectionName;
        si.addMember("budget") <<= am.MovieBudget;
        si.addMember("revenue") <<= am.MovieRevenue;
        si.addMember("genres") <<= am.MovieGenres;
        si.addMember("homepage") <<= am.MovieHomepage;
        si.addMember("release_date") <<= am.MovieReleaseDate;
        si.addMember("runtime") <<= am.MovieRuntime;
        si.addMember("popularity") <<= am.MoviePopularity;
        si.addMember("vote_average") <<= am.MovieVoteAverage;
        si.addMember("poster") <<= am.MoviePoster;
        si.addMember("fanart") <<= am.MovieFanart;
        si.addMember("collection_poster") <<= am.MovieCollectionPoster;
        si.addMember("collection_fanart") <<= am.MovieCollectionFanart;
     }
     si.addMember("actors") <<= am.Actors;
  }
}
