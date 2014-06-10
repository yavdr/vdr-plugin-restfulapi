#include "recordings.h"
using namespace std;

void RecordingsResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  bool found = false;

  if (request.method() == "OPTIONS") {
     return;
  }

  if ((int)request.url().find("/recordings/play") == 0 ) {
     if (request.method() == "POST") {
        playRecording(out, request, reply);
        reply.addHeader("Content-Type", "text/plain; charset=utf-8");
     } else {
        reply.httpReturn(501, "Only POST method is supported by the /recordings/play service.");
     }
     found = true;
  }  
  
  else if ((int)request.url().find("/recordings/rewind") == 0 ) {
     if (request.method() == "POST") {
        rewindRecording(out, request, reply);
        reply.addHeader("Content-Type", "text/plain; charset=utf-8");
     } else {
        reply.httpReturn(501, "Only POST method is supported by the /recordings/rewind service.");
     }
     found = true;
  }  

  else if ((int)request.url().find("/recordings/cut") == 0 ) {
     if (request.method() == "GET") {
        showCutterStatus(out, request, reply);
     } else if (request.method() == "POST") {
        cutRecording(out, request, reply); 
     } else {
        reply.httpReturn(501, "Only GET and POST methods are supported by the /recordings/cut service.");
     }
     found = true;
  }

  else if ((int)request.url().find("/recordings/marks") == 0 ) {
     if (request.method() == "DELETE") {
        deleteMarks(out, request, reply);
     } else if (request.method() == "POST") {
        saveMarks(out, request, reply);
     } else {
        reply.httpReturn(501, "Only DELETE and POST methods are supported by the /recordings/marks service.");
     }
     found = true;
  }

  else if ((int)request.url().find("/recordings/move") == 0 ) {
     if (request.method() == "POST") {
        moveRecording(out, request, reply);
     } else {
        reply.httpReturn(501, "Only POST method is supported by the /recordings/move service.");
     }
     found = true;
  }
  
  else if ((int) request.url().find("/recordings/delete") == 0 ) {
     if (request.method() == "POST") {
        deleteRecordingByName(out, request, reply);
     } else if (request.method() == "DELETE") {
        deleteRecordingByName(out, request, reply);
     } else {
        reply.httpReturn(501, "Only POST and DELETE methods are supported by the /recordings/delete service.");
     }
     found = true;
  }
  
  else if ((int) request.url().find("/recordings/byname") == 0 ) {
     if (request.method() == "GET") {
        showRecordingByName(out, request, reply);
     } else {
        reply.httpReturn(501, "Only GET method is supported by the /recordings/delete service.");
     }
     found = true;
  }

  // original /recordings service
  else if ((int) request.url().find("/recordings") == 0 ) {
     if (request.method() == "GET") {
        showRecordings(out, request, reply);
     } else if (request.method() == "DELETE") {
        deleteRecording(out, request, reply);
     } else {
        reply.httpReturn(501, "Only GET and DELETE methods are supported by the /recordings service.");
     }
     found = true;
  }

  if (!found) {
     reply.httpReturn(403, "Service not found");
  }
}

void RecordingsResponder::playRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/play", request);
  int recording_number = q.getParamAsInt(0);
  cThreadLock RecordingsLock(&Recordings);
  if ( recording_number < 0 || recording_number >= Recordings.Count() ) {
     reply.httpReturn(404, "Wrong recording number!");
  } else {
     cRecording* recording = Recordings.Get(recording_number);
     if ( recording != NULL ) {
        TaskScheduler::get()->SwitchableRecording(recording);
     } else {
        reply.httpReturn(404, "Wrong recording number!");
     }
  }
}

void RecordingsResponder::rewindRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/rewind", request);
  int recording_number = q.getParamAsInt(0);
  cThreadLock RecordingsLock(&Recordings);
  if ( recording_number < 0 || recording_number >= Recordings.Count() ) {
     reply.httpReturn(404, "Wrong recording number!");
  } else {
     cRecording* recording = Recordings.Get(recording_number);
     if ( recording != NULL ) {
        TaskScheduler::get()->SetRewind(true);
        TaskScheduler::get()->SwitchableRecording(recording);
     } else {
        reply.httpReturn(404, "Wrong recording number!");
     }
  }
}

/* move or copy recording */
void RecordingsResponder::moveRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/move", request);
  StreamExtension s(&out);
  string source = q.getBodyAsString("source");
  string target = q.getBodyAsString("target");
  string directory = q.getBodyAsString("directory");
  bool copy_only = q.getBodyAsBool("copy_only");
  if (!copy_only)
     cThreadLock RecordingsLock(&Recordings);
  if (source.length() > 0 && target.length() > 0) {
     if (access(source.c_str(), F_OK) == 0) {
        cRecording* recording = Recordings.GetByName(source.c_str());
        if (recording) {
           string filename = directory.empty() ? target : StringExtension::replace(directory, "/", "~") + "~" + target;
           string newname = VdrExtension::MoveRecording(recording, VdrExtension::FileSystemExchangeChars(filename.c_str(), true), copy_only);
           if (newname.length() > 0) {
              cRecording* new_recording = Recordings.GetByName(newname.c_str());
              if (new_recording) {
                 RecordingList* recordingList;
                 bool read_marks = false;

                 if ( q.isFormat(".json") ) {
                    reply.addHeader("Content-Type", "application/json; charset=utf-8");
                    recordingList = (RecordingList*)new JsonRecordingList(&out, read_marks);
                 } else if ( q.isFormat(".html") ) {
                    reply.addHeader("Content-Type", "text/html; charset=utf-8");
                    recordingList = (RecordingList*)new HtmlRecordingList(&out, read_marks);
                    recordingList->init();
                 } else if ( q.isFormat(".xml") )  {
                    reply.addHeader("Content-Type", "text/xml; charset=utf-8");
                    recordingList = (RecordingList*)new XmlRecordingList(&out, read_marks);
                    recordingList->init();
                 } else {
                    reply.httpReturn(502, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
                    return;
                 }

                 cThreadLock RecordingsLock(&Recordings);
                 for (int i = 0; i < Recordings.Count(); i++) {
                     cRecording* tmp_recording = Recordings.Get(i);
                     if (strcmp(new_recording->FileName(), tmp_recording->FileName()) == 0)
                        recordingList->addRecording(tmp_recording, i);
                 }
                 recordingList->setTotal(Recordings.Count());
                 recordingList->finish();
                 delete recordingList;
              } else {
                 LOG_ERROR_STR(newname.c_str());
              }
           } else {
              LOG_ERROR_STR(source.c_str());
              reply.httpReturn(503, "File copy failed!");
           }
        } else {
           reply.httpReturn(504, "Recording not found!");
        }
     } else {
        reply.httpReturn(504, "Path is invalid!");
     }
  } else {
     reply.httpReturn(404, "Missing file name!");
  }
}


/* get recording by file name */
void RecordingsResponder::showRecordingByName(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/byname", request);
  string recording_file = q.getOptionAsString("file");
  bool read_marks = q.getOptionAsString("marks") == "true";
  cThreadLock RecordingsLock(&Recordings);
  if (recording_file.length() > 0) {
     cRecording* recording = Recordings.GetByName(recording_file.c_str());

     if (recording) {
        RecordingList* recordingList;

        if ( q.isFormat(".json") ) {
           reply.addHeader("Content-Type", "application/json; charset=utf-8");
           recordingList = (RecordingList*)new JsonRecordingList(&out, read_marks);
        } else if ( q.isFormat(".html") ) {
           reply.addHeader("Content-Type", "text/html; charset=utf-8");
           recordingList = (RecordingList*)new HtmlRecordingList(&out, read_marks);
           recordingList->init();
        } else if ( q.isFormat(".xml") )  {
           reply.addHeader("Content-Type", "text/xml; charset=utf-8");
           recordingList = (RecordingList*)new XmlRecordingList(&out, read_marks);
           recordingList->init();
        } else {
           reply.httpReturn(502, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
           return;
        }

        cThreadLock RecordingsLock(&Recordings);
        for (int i = 0; i < Recordings.Count(); i++) {
            cRecording* tmp_recording = Recordings.Get(i);
            if (strcmp(recording->FileName(), tmp_recording->FileName()) == 0)
               recordingList->addRecording(tmp_recording, i);
        }
        recordingList->setTotal(Recordings.Count());
        recordingList->finish();
        delete recordingList;
     } else {
        LOG_ERROR_STR(recording_file.c_str());
        reply.httpReturn(504, "Recording not found!");
     }
  } else {
     reply.httpReturn(404, "No filename!");
  }
}

/* delete recording by file name */
void RecordingsResponder::deleteRecordingByName(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/delete", request);
  string recording_file = q.getBodyAsString("file");
  cThreadLock RecordingsLock(&Recordings);
  if (recording_file.length() > 0) {
     cRecording* delRecording = Recordings.GetByName(recording_file.c_str());
     if (delRecording->Delete()) {
        Recordings.DelByName(delRecording->FileName());
     }
  } else {
     reply.httpReturn(404, "No recording file!");
  }
}

void RecordingsResponder::deleteRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings", request);
  int recording_number = q.getParamAsInt(0);
  cThreadLock RecordingsLock(&Recordings);
  if ( recording_number < 0 || recording_number >= Recordings.Count() ) { 
     reply.httpReturn(404, "Wrong recording number!");
  } else {
     cRecording* delRecording = Recordings.Get(recording_number);
     if ( delRecording->Delete() ) {
        Recordings.DelByName(delRecording->FileName());
     }
  }
}

void RecordingsResponder::showRecordings(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings", request);
  RecordingList* recordingList;
  bool read_marks = q.getOptionAsString("marks") == "true";

  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     recordingList = (RecordingList*)new JsonRecordingList(&out, read_marks);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     recordingList = (RecordingList*)new HtmlRecordingList(&out, read_marks);
     recordingList->init();
  } else if ( q.isFormat(".xml") )  {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     recordingList = (RecordingList*)new XmlRecordingList(&out, read_marks);
     recordingList->init();
  } else {
     reply.httpReturn(404, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  if ( start_filter >= 0 && limit_filter >= 1 ) {
     recordingList->activateLimit(start_filter, limit_filter);
  }

  int requested_item = q.getParamAsInt(0);
  cThreadLock RecordingsLock(&Recordings);
  if ( requested_item < 0 ) {
     for (int i = 0; i < Recordings.Count(); i++)
        recordingList->addRecording(Recordings.Get(i), i);
  } else if ( requested_item < Recordings.Count() )
     recordingList->addRecording(Recordings.Get(requested_item), requested_item);
  recordingList->setTotal(Recordings.Count());

  recordingList->finish();
  delete recordingList;
}

void RecordingsResponder::saveMarks(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/marks", request);
  int recording = q.getParamAsInt(0);
  JsonArray* jsonArray = q.getBodyAsArray("marks");

  if (jsonArray == NULL) {
     reply.httpReturn(503, "Marks in HTTP-Body are missing.");
  }

  cThreadLock RecordingsLock(&Recordings);
  if (recording < 0 || recording >= Recordings.Count()) {
     reply.httpReturn(504, "Recording number missing or invalid.");
  } else {
     vector< string > marks;

     for (int i = 0; i < jsonArray->CountItem(); i++) {
        JsonBase* jsonBase = jsonArray->GetItem(i);
        if (jsonBase->IsBasicValue()) {
           JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
           if (jsonBasicValue->IsString()) {
              marks.push_back(jsonBasicValue->ValueAsString());
           }
        }
     }

     VdrMarks::get()->saveMarks(Recordings.Get(recording), marks);
  }
}

void RecordingsResponder::deleteMarks(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/marks", request);
  int rec_number = q.getParamAsInt(0);
  cThreadLock RecordingsLock(&Recordings);
  if (rec_number >= 0 && rec_number < Recordings.Count()) {
     cRecording* recording = Recordings.Get(rec_number);
     if (VdrMarks::get()->deleteMarks(recording)) {
        return;
     }
  }
  reply.httpReturn(503, "Deleting marks failed.");
}

void RecordingsResponder::cutRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/cut", request);
  int rec_number = q.getParamAsInt(0);
  cThreadLock RecordingsLock(&Recordings);
  if (rec_number >= 0 && rec_number < Recordings.Count()) {
     cRecording* recording = Recordings.Get(rec_number);
#if APIVERSNUM > 20101
     if (RecordingsHandler.GetUsage(recording->FileName()) != ruNone) {
#else
     if (cCutter::Active()) {
#endif
        reply.httpReturn(504, "VDR Cutter currently busy.");
     } else {
#if APIVERSNUM > 20101
        RecordingsHandler.Add(ruCut, recording->FileName());
#else
        cCutter::Start(recording->FileName());
#endif
     }
     return;
  }
  reply.httpReturn(503, "Cutting recordings failed.");
}

void RecordingsResponder::showCutterStatus(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/cut", request);
  StreamExtension s(&out);

#if APIVERSNUM > 20101
  bool active = RecordingsHandler.Active();
#else
  bool active = cCutter::Active();
#endif

  if (q.isFormat(".html")) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     s.writeHtmlHeader("HtmlCutterStatus");
     s.write((active ? "True" : "False"));
     s.write("</body></html>");
  } else if (q.isFormat(".json")) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     cxxtools::JsonSerializer serializer(out);
     serializer.serialize(active, "active");
     serializer.finish();     
  } else if (q.isFormat(".xml")) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     s.write("<cutter xmlns=\"http://www.domain.org/restfulapi/2011/cutter-xml\">\n");
     s.write(cString::sprintf(" <param name=\"active\">%s</param>\n", (active ? "true" : "false")));
     s.write("</cutter>");
  } else {
     reply.httpReturn(502, "Only the following formats are supported: .xml, .json and .html");
  } 
}

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p)
{
  si.addMember("number") <<= p.Number;
  si.addMember("name") <<= p.Name;
  si.addMember("file_name") <<= p.FileName;
  si.addMember("relative_file_name") <<= p.RelativeFileName;
  si.addMember("is_new") <<= p.IsNew;
  si.addMember("is_edited") <<= p.IsEdited;
  si.addMember("is_pes_recording") <<= p.IsPesRecording;
  si.addMember("duration") <<= p.Duration;
  si.addMember("filesize_mb") <<= p.FileSizeMB;
  si.addMember("channel_id") <<= p.ChannelID;
  si.addMember("frames_per_second") <<= p.FramesPerSecond;
  si.addMember("marks") <<= p.Marks.marks;
  si.addMember("event_title") <<= p.EventTitle;
  si.addMember("event_short_text") <<= p.EventShortText;
  si.addMember("event_description") <<= p.EventDescription;
  si.addMember("event_start_time") <<= p.EventStartTime;
  si.addMember("event_duration") <<= p.EventDuration;
  si.addMember("additional_media") <<= p.Scraper;
  si.addMember("poster") <<= p.ScraperPoster;
//  si.addMember("fanart") <<= p.ScraperFanart;
  si.addMember("banner") <<= p.ScraperBanner;
}

RecordingList::RecordingList(ostream *out, bool _read_marks)
{
  s = new StreamExtension(out);
  read_marks = _read_marks;
}

RecordingList::~RecordingList()
{
  delete s;
}

void HtmlRecordingList::init()
{
  s->writeHtmlHeader("HtmlRecordingList");
  s->write("<ul>");
}

void HtmlRecordingList::addRecording(cRecording* recording, int nr)
{
  if ( filtered() ) return;
  s->write("<li>");
  s->write((char*)recording->Name());
}

void HtmlRecordingList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonRecordingList::addRecording(cRecording* recording, int nr)
{
  if ( filtered() ) return;

  cxxtools::String empty = StringExtension::UTF8Decode("");
  cxxtools::String eventTitle = empty;
  cxxtools::String eventShortText = empty;
  cxxtools::String eventDescription = empty;
  int eventStartTime = -1;
  int eventDuration = -1;
    
  cMovie movie;
  cSeries series;
  ScraperGetEventType call;
  bool hasAdditionalMedia = false;
  bool isMovie = false;
  bool isSeries = false;
    
  cEvent* event = (cEvent*)recording->Info()->GetEvent();
  
  if (event != NULL)
  {
     if (event->Title())         { eventTitle = StringExtension::UTF8Decode(event->Title()); }
     if (event->ShortText())     { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
     if (event->Description())   { eventDescription = StringExtension::UTF8Decode(event->Description()); }
     if (event->StartTime() > 0) { eventStartTime = event->StartTime(); }
     if (event->Duration() > 0)  { eventDuration = event->Duration(); }
        
     static cPlugin *pScraper = GetScraperPlugin();
     if (pScraper) {
        ScraperGetEventType call;
        call.recording = recording;
        int seriesId = 0;
        int episodeId = 0;
        int movieId = 0;
        if (pScraper->Service("GetEventType", &call)) {
           //esyslog("restfulapi: Type detected: %d, seriesId %d, episodeId %d, movieId %d", call.type, call.seriesId, call.episodeId, call.movieId);
           seriesId = call.seriesId;
           episodeId = call.episodeId;
           movieId = call.movieId;
        }
        if (seriesId > 0) {
           series.seriesId = seriesId;
           series.episodeId = episodeId;
           if (pScraper->Service("GetSeries", &series)) {
              hasAdditionalMedia = true;
              isSeries = true;
           }
        } else if (movieId > 0) {
           movie.movieId = movieId;
           if (pScraper->Service("GetMovie", &movie)) {
              hasAdditionalMedia = true;
              isMovie = true;
           }
        }
     }
  }

  SerRecording serRecording;
  serRecording.Number = nr;
  serRecording.Name = StringExtension::encodeToJson(recording->Name());
  serRecording.FileName = StringExtension::UTF8Decode(recording->FileName());
  serRecording.RelativeFileName = StringExtension::UTF8Decode(VdrExtension::getRelativeVideoPath(recording).c_str());
  serRecording.IsNew = recording->IsNew();
  serRecording.IsEdited = recording->IsEdited();

  #if APIVERSNUM >= 10703
  serRecording.IsPesRecording = recording->IsPesRecording();
  serRecording.FramesPerSecond = recording->FramesPerSecond();
  #else
  serRecording.IsPesRecording = true;
  serRecording.FramesPerSecond = FRAMESPERSEC;
  #endif

  serRecording.Duration = VdrExtension::RecordingLengthInSeconds(recording);
  serRecording.FileSizeMB = recording->FileSizeMB();
  serRecording.ChannelID =  StringExtension::UTF8Decode((string) recording->Info()->ChannelID().ToString());

  serRecording.EventTitle = eventTitle;
  serRecording.EventShortText = eventShortText;
  serRecording.EventDescription = eventDescription;
  serRecording.EventStartTime = eventStartTime;
  serRecording.EventDuration = eventDuration;

  if (hasAdditionalMedia) {
     if (isSeries) {
        serRecording.Scraper = StringExtension::UTF8Decode("series");
        if (series.posters.size() > 0) { /*
           int posters = series.posters.size();
           for (int i = 0; i < posters;i++) {
               serRecording.TvScraperPoster = StringExtension::UTF8Decode(series.posters[i].path);
           } */
           serRecording.ScraperPoster = StringExtension::UTF8Decode(series.posters[0].path);
        }
        if (series.banners.size() > 0) {
           serRecording.ScraperBanner = StringExtension::UTF8Decode(series.banners[0].path);
        }
     } else if (isMovie) {
        serRecording.Scraper = StringExtension::UTF8Decode("movie");
        if ((movie.poster.width > 0) && (movie.poster.height > 0) && (movie.poster.path.size() > 0)) {
           serRecording.ScraperPoster = StringExtension::UTF8Decode(movie.poster.path);
        }
     }
  }

  SerMarks serMarks;
  if (read_marks) {
     serMarks.marks = VdrMarks::get()->readMarks(recording);
  }

  serRecording.Marks = serMarks;

  serRecordings.push_back(serRecording);
}

void JsonRecordingList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(serRecordings, "recordings");
  serializer.serialize(serRecordings.size(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlRecordingList::init()
{
  s->writeXmlHeader();
  s->write("<recordings xmlns=\"http://www.domain.org/restfulapi/2011/recordings-xml\">\n");
}

void XmlRecordingList::addRecording(cRecording* recording, int nr)
{
  if ( filtered() ) return;

  string eventTitle = "";
  string eventShortText = "";
  string eventDescription = "";
  int eventStartTime = -1;
  int eventDuration = -1;

  cMovie movie;
  cSeries series;
  ScraperGetEventType call;
  bool hasAdditionalMedia = false;
  bool isMovie = false;
  bool isSeries = false;

  cEvent* event = (cEvent*)recording->Info()->GetEvent();

  if (event != NULL)
  {
     if (event->Title())         { eventTitle = event->Title(); }
     if (event->ShortText())     { eventShortText = event->ShortText(); }
     if (event->Description())   { eventDescription = event->Description(); }
     if (event->StartTime() > 0) { eventStartTime = event->StartTime(); }
     if (event->Duration() > 0)  { eventDuration = event->Duration(); }

     static cPlugin *pScraper = GetScraperPlugin();
     if (pScraper) {
        ScraperGetEventType call;
        call.recording = recording;
        int seriesId = 0;
        int episodeId = 0;
        int movieId = 0;
        if (pScraper->Service("GetEventType", &call)) {
           //esyslog("restfulapi: Type detected: %d, seriesId %d, episodeId %d, movieId %d", call.type, call.seriesId, call.episodeId, call.movieId);
           seriesId = call.seriesId;
           episodeId = call.episodeId;
           movieId = call.movieId;
        }
        if (seriesId > 0) {
           series.seriesId = seriesId;
           series.episodeId = episodeId;
           if (pScraper->Service("GetSeries", &series)) {
              hasAdditionalMedia = true;
              isSeries = true;
           }
        } else if (movieId > 0) {
           movie.movieId = movieId;
           if (pScraper->Service("GetMovie", &movie)) {
              hasAdditionalMedia = true;
              isMovie = true;
           }
        }
     }
  }

  s->write(" <recording>\n");
  s->write(cString::sprintf("  <param name=\"number\">%i</param>\n", nr));
  s->write(cString::sprintf("  <param name=\"name\">%s</param>\n", StringExtension::encodeToXml(recording->Name()).c_str() ));
  s->write(cString::sprintf("  <param name=\"filename\">%s</param>\n", StringExtension::encodeToXml(recording->FileName()).c_str()) );
  s->write(cString::sprintf("  <param name=\"relative_filename\">%s</param>\n", StringExtension::encodeToXml(VdrExtension::getRelativeVideoPath(recording).c_str()).c_str()));
  s->write(cString::sprintf("  <param name=\"is_new\">%s</param>\n", recording->IsNew() ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"is_edited\">%s</param>\n", recording->IsEdited() ? "true" : "false" ));

  #if APIVERSNUM >= 10703
  s->write(cString::sprintf("  <param name=\"is_pes_recording\">%s</param>\n", recording->IsPesRecording() ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"frames_per_second\">%f</param>\n", recording->FramesPerSecond()));
  #else
  s->write(cString::sprintf("  <param name=\"is_pes_recording\">%s</param>\n", true ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"frames_per_second\">%i</param>\n", FRAMESPERSEC));
  #endif
  s->write(cString::sprintf("  <param name=\"duration\">%i</param>\n", VdrExtension::RecordingLengthInSeconds(recording)));
  s->write(cString::sprintf("  <param name=\"filesize_mb\">%i</param>\n", recording->FileSizeMB()));
  s->write(cString::sprintf("  <param name=\"channel_id\">%s</param>\n", StringExtension::encodeToXml((string) recording->Info()->ChannelID().ToString()).c_str()));

  if (read_marks) {
     s->write("  <param name=\"marks\">\n");
     vector< string > marks = VdrMarks::get()->readMarks(recording);
     for(int i=0;i<(int)marks.size();i++) {
        s->write(cString::sprintf("   <mark>%s</mark>\n", marks[i].c_str()));
     }
     s->write("  </param>\n");
  }

  s->write(cString::sprintf("  <param name=\"event_title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str()));
  s->write(cString::sprintf("  <param name=\"event_short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str()));
  s->write(cString::sprintf("  <param name=\"event_description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str()));
  s->write(cString::sprintf("  <param name=\"event_start_time\">%i</param>\n", eventStartTime));
  s->write(cString::sprintf("  <param name=\"event_duration\">%i</param>\n", eventDuration));

  if (hasAdditionalMedia) {
     if (isSeries) {
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
           s->write(cString::sprintf("    <episode_image>%s</episode_image>\n", StringExtension::encodeToXml(series.episode.episodeImage.path).c_str()));
        }
         
        if (series.actors.size() > 0) {
           int _actors = series.actors.size();
           for (int i = 0; i < _actors; i++) {
               s->write(cString::sprintf("    <actor name=\"%s\" role=\"%s\" thumb=\"%s\"/>\n",
                                         StringExtension::encodeToXml(series.actors[i].name).c_str(),
                                         StringExtension::encodeToXml(series.actors[i].role).c_str(),
                                         StringExtension::encodeToXml(series.actors[i].actorThumb.path).c_str() ));
           }
        }
        if (series.posters.size() > 0) {
           int _posters = series.posters.size();
           for (int i = 0; i < _posters; i++) {
               if ((series.posters[i].width > 0) && (series.posters[i].height > 0))
                  s->write(cString::sprintf("    <poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                            StringExtension::encodeToXml(series.posters[i].path).c_str(), series.posters[i].width, series.posters[i].height));
           }
        }
        if (series.banners.size() > 0) {
           int _banners = series.banners.size();
           for (int i = 0; i < _banners; i++) {
               if ((series.banners[i].width > 0) && (series.banners[i].height > 0))
                  s->write(cString::sprintf("    <banner path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                            StringExtension::encodeToXml(series.banners[i].path).c_str(), series.banners[i].width, series.banners[i].height));
           }
        }
        if (series.fanarts.size() > 0) {
           int _fanarts = series.fanarts.size();
           for (int i = 0; i < _fanarts; i++) {
               if ((series.fanarts[i].width > 0) && (series.fanarts[i].height > 0))
                  s->write(cString::sprintf("    <fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                            StringExtension::encodeToXml(series.fanarts[i].path).c_str(), series.fanarts[i].width, series.fanarts[i].height));
           }
        }
        if ((series.seasonPoster.width > 0) && (series.seasonPoster.height > 0) && (series.seasonPoster.path.size() > 0)) {
           s->write(cString::sprintf("    <season_poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(series.seasonPoster.path).c_str(), series.seasonPoster.width, series.seasonPoster.height));
        }
        s->write("  </param>\n");

     } else if (isMovie) {
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
        if (movie.adult) {
           s->write("    <adult>true</adult>\n");
        }
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
                                     StringExtension::encodeToXml(movie.poster.path).c_str(), movie.poster.width, movie.poster.height));
        }
        if ((movie.fanart.width > 0) && (movie.fanart.height > 0) && (movie.fanart.path.size() > 0)) {
           s->write(cString::sprintf("    <fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.fanart.path).c_str(), movie.fanart.width, movie.fanart.height));
        }
        if ((movie.collectionPoster.width > 0) && (movie.collectionPoster.height > 0) && (movie.collectionPoster.path.size() > 0)) {
           s->write(cString::sprintf("    <collection_poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.collectionPoster.path).c_str(), movie.collectionPoster.width, movie.collectionPoster.height));
        }
        if ((movie.collectionFanart.width > 0) && (movie.collectionFanart.height > 0) && (movie.collectionFanart.path.size() > 0)) {
           s->write(cString::sprintf("    <collection_fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.collectionFanart.path).c_str(), movie.collectionFanart.width, movie.collectionFanart.height));
        }
        if (movie.actors.size() > 0) {
           int _actors = movie.actors.size();
           for (int i = 0; i < _actors; i++) {
               s->write(cString::sprintf("    <actor name=\"%s\" role=\"%s\" thumb=\"%s\"/>\n",
                                         StringExtension::encodeToXml(movie.actors[i].name).c_str(),
                                         StringExtension::encodeToXml(movie.actors[i].role).c_str(),
                                         StringExtension::encodeToXml(movie.actors[i].actorThumb.path).c_str()));
           }
        }
        s->write("  </param>\n");
     }
  }
  s->write(" </recording>\n");
}

void XmlRecordingList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</recordings>");
}
