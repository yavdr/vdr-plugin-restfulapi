#include "scraper2vdr.h"
using namespace std;

/**
 * initialize scraper service
 */
Scraper2VdrService::Scraper2VdrService() {
  scraper = getScraperPlugin();
  epgImagesDir = Settings::get()->EpgImageDirectory();
};

/**
 * destroy scraper service
 */
Scraper2VdrService::~Scraper2VdrService() {
  scraper = NULL;
};

/**
 * retrieve scraper plugin
 * @return cPlugin
 */
cPlugin *Scraper2VdrService::getScraperPlugin () {

  cPlugin *pScraper = cPluginManager::GetPlugin("scraper2vdr");
  if ( !pScraper )
    pScraper = cPluginManager::GetPlugin("tvscraper");

  return pScraper;
};

/**
 * determine eventType
 * @param ScraperGetEventType &eventType
 * @return bool
 */
bool Scraper2VdrService::getEventType(ScraperGetEventType &eventType) {

  if (scraper) {
    return scraper->Service("GetEventType", &eventType);
  }

  return false;
};

/**
 * retrieve media by eventType
 * @param ScraperGetEventType &eventType
 * @param SerAdditionalMedia &am
 * @return bool
 */
bool Scraper2VdrService::getMedia(ScraperGetEventType &eventType, SerAdditionalMedia &am) {

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
 * retrieve media by eventType
 * @param ScraperGetEventType &eventType
 * @param StreamExtension* s
 * @return bool
 */
bool Scraper2VdrService::getMedia(ScraperGetEventType &eventType, StreamExtension* s) {

  if (getEventType(eventType)) {
      if (eventType.seriesId > 0) {
	  getSeriesMedia(s, eventType);
	  return true;
      } else if (eventType.movieId > 0) {
	  getMovieMedia(s, eventType);
	  return true;
      }
  }
  return false;
};

/**
 * API begin
 */

/**
 * retrieve additional media according to event
 * @param cEvent *event
 * @param SerAdditionalMedia &am
 * @return bool
 */
bool Scraper2VdrService::getMedia(cEvent *event, SerAdditionalMedia &am) {

  if (!scraper || !event) return false;

  ScraperGetEventType eventType;
  eventType.event = event;

  return getMedia(eventType, am);
};

/**
 * retrieve additional media according to event
 * @param cEvent *event
 * @param StreamExtension* s
 * @return bool
 */
bool Scraper2VdrService::getMedia(cEvent *event, StreamExtension* s) {

  if (!scraper || !event || !s) return false;

  ScraperGetEventType eventType;
  eventType.event = event;

  return getMedia(eventType, s);
};

/**
 * retrieve additional media according to recording
 * @param cRecording* recording
 * @param SerAdditionalMedia &am
 * @return bool
 */
bool Scraper2VdrService::getMedia(cRecording* recording, SerAdditionalMedia &am) {

  if (!scraper || !recording) return false;

  ScraperGetEventType eventType;
  eventType.recording = recording;

  return getMedia(eventType, am);
};

/**
 * retrieve additional media according to recording
 * @param cRecording* recording
 * @param StreamExtension* s
 * @return bool
 */
bool Scraper2VdrService::getMedia(cRecording* recording, StreamExtension* s) {

  if (!scraper || !recording || !s) return false;

  ScraperGetEventType eventType;
  eventType.recording = recording;

  return getMedia(eventType, s);
};

/**
 * API end
 */

/**
 * strip filesystem path from image path
 * @param string path
 * return string
 */
string Scraper2VdrService::cleanImagePath(string path) {

  path = StringExtension::replace(path, epgImagesDir, "");
  path.erase(0, path.find_first_not_of("/"));
  return path;
};

/**
 * enrich additional media structure with series data
 * @param SerAdditionalMedia &am
 * @param ScraperGetEventType &eventType
 * @return void
 */
void Scraper2VdrService::getSeriesMedia(SerAdditionalMedia &am, ScraperGetEventType &eventType) {

  cSeries series;
  series.seriesId = eventType.seriesId;
  series.episodeId = eventType.episodeId;

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
    am.EpisodeImage = StringExtension::UTF8Decode(cleanImagePath(series.episode.episodeImage.path));

    if (series.actors.size() > 0) {
       int _actors = series.actors.size();
       for (int i = 0; i < _actors; i++) {
	   struct SerActor actor;
	   actor.Name = StringExtension::UTF8Decode(series.actors[i].name);
	   actor.Role = StringExtension::UTF8Decode(series.actors[i].role);
	   actor.Thumb = StringExtension::UTF8Decode(cleanImagePath(series.actors[i].actorThumb.path));
	   am.Actors.push_back(actor);
       }
    }

    if (series.posters.size() > 0) {
       int _posters = series.posters.size();
       for (int i = 0; i < _posters; i++) {
	   if ((series.posters[i].width > 0) && (series.posters[i].height > 0)) {
	      struct SerImage poster;
	      poster.Path = StringExtension::UTF8Decode(cleanImagePath(series.posters[i].path));
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
	      banner.Path = StringExtension::UTF8Decode(cleanImagePath(series.banners[i].path));
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
	      fanart.Path = StringExtension::UTF8Decode(cleanImagePath(series.fanarts[i].path));
	      fanart.Width = series.fanarts[i].width;
	      fanart.Height = series.fanarts[i].height;
	      am.Fanarts.push_back(fanart);
	   }
       }
    }
  }
};

/**
 * enrich additional media structure with series data
 * @param StreamExtension* s
 * @param ScraperGetEventType &eventType
 * @return void
 */
void Scraper2VdrService::getSeriesMedia(StreamExtension* s, ScraperGetEventType &eventType) {

  cSeries series;
  series.seriesId = eventType.seriesId;
  series.episodeId = eventType.episodeId;

  if (scraper->Service("GetSeries", &series)) {
    s->write("  <param name=\"additional_media\" type=\"series\">\n");
    s->write(cString::sprintf("    <series_id>%i</series_id>\n", series.seriesId));
    if (series.episodeId > 0) {
	s->write(cString::sprintf("    <episode_id>%i</episode_id>\n", series.episodeId));
    }
    if (series.name != "") {
	s->write(cString::sprintf("    <name>%s</name>\n", StringExtension::encodeToXml(series.name).c_str()));
    }
    if (series.overview != "") {
	s->write(cString::sprintf("    <overview>%s</overview>\n", StringExtension::encodeToXml(series.overview).c_str()));
    }
    if (series.firstAired != "") {
	s->write(cString::sprintf("    <first_aired>%s</first_aired>\n", StringExtension::encodeToXml(series.firstAired).c_str()));
    }
    if (series.network != "") {
	s->write(cString::sprintf("    <network>%s</network>\n", StringExtension::encodeToXml(series.network).c_str()));
    }
    if (series.genre != "") {
	s->write(cString::sprintf("    <genre>%s</genre>\n", StringExtension::encodeToXml(series.genre).c_str()));
    }
    if (series.rating > 0) {
	s->write(cString::sprintf("    <rating>%.2f</rating>\n", series.rating));
    }
    if (series.status != "") {
	s->write(cString::sprintf("    <status>%s</status>\n", StringExtension::encodeToXml(series.status).c_str()));
    }

    if (series.episode.number > 0) {
       s->write(cString::sprintf("    <episode_number>%i</episode_number>\n", series.episode.number));
       s->write(cString::sprintf("    <episode_season>%i</episode_season>\n", series.episode.season));
       s->write(cString::sprintf("    <episode_name>%s</episode_name>\n", StringExtension::encodeToXml(series.episode.name).c_str()));
       s->write(cString::sprintf("    <episode_first_aired>%s</episode_first_aired>\n", StringExtension::encodeToXml(series.episode.firstAired).c_str()));
       s->write(cString::sprintf("    <episode_guest_stars>%s</episode_guest_stars>\n", StringExtension::encodeToXml(series.episode.guestStars).c_str()));
       s->write(cString::sprintf("    <episode_overview>%s</episode_overview>\n", StringExtension::encodeToXml(series.episode.overview).c_str()));
       s->write(cString::sprintf("    <episode_rating>%.2f</episode_rating>\n", series.episode.rating));
       s->write(cString::sprintf("    <episode_image>%s</episode_image>\n", StringExtension::encodeToXml(cleanImagePath(series.episode.episodeImage.path)).c_str()));
    }

    if (series.actors.size() > 0) {
       int _actors = series.actors.size();
       for (int i = 0; i < _actors; i++) {
	   s->write(cString::sprintf("    <actor name=\"%s\" role=\"%s\" thumb=\"%s\"/>\n",
				     StringExtension::encodeToXml(series.actors[i].name).c_str(),
				     StringExtension::encodeToXml(series.actors[i].role).c_str(),
				     StringExtension::encodeToXml(cleanImagePath(series.actors[i].actorThumb.path)).c_str() ));
       }
    }
    if (series.posters.size() > 0) {
       int _posters = series.posters.size();
       for (int i = 0; i < _posters; i++) {
	   if ((series.posters[i].width > 0) && (series.posters[i].height > 0))
	      s->write(cString::sprintf("    <poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
					StringExtension::encodeToXml(cleanImagePath(series.posters[i].path)).c_str(), series.posters[i].width, series.posters[i].height));
       }
    }
    if (series.banners.size() > 0) {
       int _banners = series.banners.size();
       for (int i = 0; i < _banners; i++) {
	   if ((series.banners[i].width > 0) && (series.banners[i].height > 0))
	      s->write(cString::sprintf("    <banner path=\"%s\" width=\"%i\" height=\"%i\" />\n",
					StringExtension::encodeToXml(cleanImagePath(series.banners[i].path)).c_str(), series.banners[i].width, series.banners[i].height));
       }
    }
    if (series.fanarts.size() > 0) {
       int _fanarts = series.fanarts.size();
       for (int i = 0; i < _fanarts; i++) {
	   if ((series.fanarts[i].width > 0) && (series.fanarts[i].height > 0))
	      s->write(cString::sprintf("    <fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
					StringExtension::encodeToXml(cleanImagePath(series.fanarts[i].path)).c_str(), series.fanarts[i].width, series.fanarts[i].height));
       }
    }
    if ((series.seasonPoster.width > 0) && (series.seasonPoster.height > 0) && (series.seasonPoster.path.size() > 0)) {
       s->write(cString::sprintf("    <season_poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
				 StringExtension::encodeToXml(cleanImagePath(series.seasonPoster.path)).c_str(), series.seasonPoster.width, series.seasonPoster.height));
    }
    s->write("  </param>\n");
  }
};

/**
 * enrich additional media structure with movie data
 * @param SerAdditionalMedia &am
 * @param ScraperGetEventType &eventType
 * @return void
 */
void Scraper2VdrService::getMovieMedia(SerAdditionalMedia &am, ScraperGetEventType &eventType) {

  cMovie movie;
  movie.movieId = eventType.movieId;

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
      am.MoviePoster = StringExtension::UTF8Decode(cleanImagePath(movie.poster.path));
      am.MovieFanart = StringExtension::UTF8Decode(cleanImagePath(movie.fanart.path));
      am.MovieCollectionPoster = StringExtension::UTF8Decode(cleanImagePath(movie.collectionPoster.path));
      am.MovieCollectionFanart = StringExtension::UTF8Decode(cleanImagePath(movie.collectionFanart.path));
      if (movie.actors.size() > 0) {
         int _actors = movie.actors.size();
         for (int i = 0; i < _actors; i++) {
             struct SerActor actor;
             actor.Name = StringExtension::UTF8Decode(movie.actors[i].name);
             actor.Role = StringExtension::UTF8Decode(movie.actors[i].role);
             actor.Thumb = StringExtension::encodeToJson(cleanImagePath(movie.actors[i].actorThumb.path)).c_str();
             am.Actors.push_back(actor);
         }
      }
  }
};

/**
 * enrich additional media structure with movie data
 * @param StreamExtension* s
 * @param ScraperGetEventType &eventType
 * @return void
 */
void Scraper2VdrService::getMovieMedia(StreamExtension* s, ScraperGetEventType &eventType) {

  cMovie movie;
  movie.movieId = eventType.movieId;

  if (scraper->Service("GetMovie", &movie)) {
    s->write("  <param name=\"additional_media\" type=\"movie\">\n");
    s->write(cString::sprintf("    <movie_id>%i</movie_id>\n", movie.movieId));
    if (movie.title != "") {
       s->write(cString::sprintf("    <title>%s</title>\n", StringExtension::encodeToXml(movie.title).c_str()));
    }
    if (movie.originalTitle != "") {
       s->write(cString::sprintf("    <original_title>%s</original_title>\n", StringExtension::encodeToXml(movie.originalTitle).c_str()));
    }
    if (movie.tagline != "") {
       s->write(cString::sprintf("    <tagline>%s</tagline>\n", StringExtension::encodeToXml(movie.tagline).c_str()));
    }
    if (movie.overview != "") {
       s->write(cString::sprintf("    <overview>%s</overview>\n", StringExtension::encodeToXml(movie.overview).c_str()));
    }
    s->write(cString::sprintf("    <adult>%s</adult>\n", (movie.adult ? "true" : "false")));
    if (movie.collectionName != "") {
       s->write(cString::sprintf("    <collection_name>%s</collection_name>\n", StringExtension::encodeToXml(movie.collectionName).c_str()));
    }
    if (movie.budget > 0) {
       s->write(cString::sprintf("    <budget>%i</budget>\n", movie.budget));
    }
    if (movie.revenue > 0) {
       s->write(cString::sprintf("    <revenue>%i</revenue>\n", movie.revenue));
    }
    if (movie.genres != "") {
       s->write(cString::sprintf("    <genres>%s</genres>\n", StringExtension::encodeToXml(movie.genres).c_str()));
    }
    if (movie.homepage != "") {
       s->write(cString::sprintf("    <homepage>%s</homepage>\n", StringExtension::encodeToXml(movie.homepage).c_str()));
    }
    if (movie.releaseDate != "") {
       s->write(cString::sprintf("    <release_date>%s</release_date>\n", StringExtension::encodeToXml(movie.releaseDate).c_str()));
    }
    if (movie.runtime > 0) {
       s->write(cString::sprintf("    <runtime>%i</runtime>\n", movie.runtime));
    }
    if (movie.popularity > 0) {
       s->write(cString::sprintf("    <popularity>%.2f</popularity>\n", movie.popularity));
    }
    if (movie.voteAverage > 0) {
       s->write(cString::sprintf("    <vote_average>%.2f</vote_average>\n", movie.voteAverage));
    }
    if ((movie.poster.width > 0) && (movie.poster.height > 0) && (movie.poster.path.size() > 0)) {
       s->write(cString::sprintf("    <poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
				 StringExtension::encodeToXml(cleanImagePath(movie.poster.path)).c_str(), movie.poster.width, movie.poster.height));
    }
    if ((movie.fanart.width > 0) && (movie.fanart.height > 0) && (movie.fanart.path.size() > 0)) {
       s->write(cString::sprintf("    <fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
				 StringExtension::encodeToXml(cleanImagePath(movie.fanart.path)).c_str(), movie.fanart.width, movie.fanart.height));
    }
    if ((movie.collectionPoster.width > 0) && (movie.collectionPoster.height > 0) && (movie.collectionPoster.path.size() > 0)) {
       s->write(cString::sprintf("    <collection_poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
				 StringExtension::encodeToXml(cleanImagePath(movie.collectionPoster.path)).c_str(), movie.collectionPoster.width, movie.collectionPoster.height));
    }
    if ((movie.collectionFanart.width > 0) && (movie.collectionFanart.height > 0) && (movie.collectionFanart.path.size() > 0)) {
       s->write(cString::sprintf("    <collection_fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
				 StringExtension::encodeToXml(cleanImagePath(movie.collectionFanart.path)).c_str(), movie.collectionFanart.width, movie.collectionFanart.height));
    }
    if (movie.actors.size() > 0) {
       int _actors = movie.actors.size();
       for (int i = 0; i < _actors; i++) {
	   s->write(cString::sprintf("    <actor name=\"%s\" role=\"%s\" thumb=\"%s\"/>\n",
				     StringExtension::encodeToXml(movie.actors[i].name).c_str(),
				     StringExtension::encodeToXml(movie.actors[i].role).c_str(),
				     StringExtension::encodeToXml(cleanImagePath(movie.actors[i].actorThumb.path)).c_str()));
       }
    }
    s->write("  </param>\n");
  }
};

/**
 * respond to image requests
 * @param ostream& out
 * @param cxxtools::http::Request& request
 * @param cxxtools::http::Reply& reply
 * return void
 */
void ScraperImageResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler::addHeader(reply);

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET");
      reply.httpReturn(200, "OK");
      return;
  }
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve images use the GET method!");
     return;
  }

  double timediff = -1;
  string base = "/scraper/image/";
  string epgImagesPath = Settings::get()->EpgImageDirectory();
  string url = request.url();

  if ( (int)url.find(base) == 0 ) {

      isyslog("restfulapi Scraper: image request url %s", request.url().c_str());

      string image = url.replace(0, base.length(), "");
      string path = epgImagesPath + (string)"/" + image;

      if (!FileExtension::get()->exists(path)) {
	  isyslog("restfulapi Scraper: image %s does not exist", request.url().c_str());
	  reply.httpReturn(404, "File not found");
	  return;
      }

      if (request.hasHeader("If-Modified-Since")) {
	  timediff = difftime(FileExtension::get()->getModifiedTime(path), FileExtension::get()->getModifiedSinceTime(request));
      }
      if (timediff > 0.0 || timediff < 0.0) {
	  string type = image.substr(image.find_last_of(".")+1);
	  string contenttype = (string)"image/" + type;
	  StreamExtension se(&out);
	  if ( se.writeBinary(path) ) {
	      isyslog("restfulapi Scraper: successfully piped image %s", request.url().c_str());
	      QueryHandler::addHeader(reply);
	      FileExtension::get()->addModifiedHeader(path, reply);
	      reply.addHeader("Content-Type", contenttype.c_str());
	  } else {
	      isyslog("restfulapi Scraper: error piping image %s", request.url().c_str());
	      reply.httpReturn(404, "File not found");
	  }
      } else {
	  isyslog("restfulapi Scraper: image %s not modified, returning 304", request.url().c_str());
	  reply.httpReturn(304, "Not-Modified");
      }
  }
};

/* ********* */


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
