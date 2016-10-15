#include "recordings.h"
using namespace std;

void RecordingsResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);

  bool found = false;

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET, POST, DELETE");
      reply.httpReturn(200, "OK");
      return;
  }


  if ((int)request.url().find("/recordings/play") == 0 ) {
     if ( request.method() == "GET" ) {
        playRecording(out, request, reply);
        reply.addHeader("Content-Type", "text/plain; charset=utf-8");
     } else if (request.method() == "POST") {
        rewindRecording(out, request, reply);
        reply.addHeader("Content-Type", "text/plain; charset=utf-8");
     } else {
        reply.httpReturn(501, "Only GET and POST method is supported by the /recordings/play service.");
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

  else if ((int)request.url().find("/recordings/editedfile") == 0 ) {
     if (request.method() == "GET") {
	 replyEditedFileName(out, request, reply);
     } else {
        reply.httpReturn(501, "Only GET method are supported by the /recordings/lastedited service.");
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

  else if ((int) request.url().find("/recordings/updates") == 0 ) {
     if (request.method() == "GET") {
        replyUpdates(out, request, reply);
     } else {
        reply.httpReturn(501, "Only GET method is supported by the /recordings/updates service.");
     }
     found = true;
  }

  else if ((int) request.url().find("/recordings/sync") == 0 ) {
     if (request.method() == "POST") {
    	 replySyncList(out, request, reply);
     } else {
        reply.httpReturn(501, "Only POST method is supported by the /recordings/sync service.");
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
        reply.httpReturn(501, "Only GET, POST and DELETE methods are supported by the /recordings service.");
     }
     found = true;
  }

  if (!found) {
     reply.httpReturn(403, "Service not found");
  }
}

const cRecording* RecordingsResponder::getRecordingByRequest(QueryHandler q) {

  int recording_number = -1;
  string pathParam;
  string fullPath;

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_READ;
    const cRecordings& recordings = *Recordings;
#else
    cThreadLock RecordingsLock(&Recordings);
    cRecordings& recordings = Recordings;
#endif

  const cRecording* recording = recordings.GetByName(q.getParamAsRecordingPath().c_str());

  if ( recording == NULL ) {

      recording_number = q.getParamAsInt(0);

      if ( recording_number >= 0 && recording_number < recordings.Count() ) {
    	  return recordings.Get(recording_number);
      }

  } else {

      return recording;
  }

  esyslog("restfulapi: requested recording not found (read)");

  return NULL;
};

cRecording* RecordingsResponder::getRecordingByRequestWrite(QueryHandler q) {

  int recording_number = -1;
  string pathParam;
  string fullPath;

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_WRITE;
    cRecordings& recordings = *Recordings;
#else
    cThreadLock RecordingsLock(&Recordings);
    cRecordings& recordings = Recordings;
#endif

    string path = q.getParamAsRecordingPath();

    esyslog("restfulapi: path: %s", path.c_str());

  cRecording* recording = recordings.GetByName(path.c_str());

  if ( recording == NULL ) {

      recording_number = q.getParamAsInt(0);

      if ( recording_number >= 0 && recording_number < recordings.Count() ) {
    	  return recordings.Get(recording_number);
      }

  } else {

      return recording;
  }

  esyslog("restfulapi: requested recording not found (write)");

  return NULL;
};

RecordingList* RecordingsResponder::getRecordingList(ostream& out, QueryHandler q, cxxtools::http::Reply& reply, bool read_marks = false) {

  RecordingList* recordingList;
  if ( q.isFormat(".json") ) {

    reply.addHeader("Content-Type", CT_JSON);
    recordingList = (RecordingList*)new JsonRecordingList(&out, read_marks);

  } else if ( q.isFormat(".html") ) {

    reply.addHeader("Content-Type", CT_HTML);
    recordingList = (RecordingList*)new HtmlRecordingList(&out, read_marks);

  } else if ( q.isFormat(".xml") )  {

    reply.addHeader("Content-Type", CT_XML);
    recordingList = (RecordingList*)new XmlRecordingList(&out, read_marks);

  } else {

    reply.httpReturn(502, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
    return NULL;
  }

  return recordingList;
};

void RecordingsResponder::playRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/play", request);
  const cRecording* recording = getRecordingByRequest(q);
  if ( recording != NULL ) {
    TaskScheduler::get()->SwitchableRecording(recording);
  } else {
    reply.httpReturn(404, "Wrong recording number or filename!");
  }
}

void RecordingsResponder::rewindRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/play", request);
  const cRecording* recording = getRecordingByRequest(q);

  if ( recording != NULL ) {
    cDevice::PrimaryDevice()->StopReplay(); // must do this first to be able to rewind the currently replayed recording
    cResumeFile ResumeFile(recording->FileName(), recording->IsPesRecording());
    ResumeFile.Delete();
    TaskScheduler::get()->SwitchableRecording(recording);
  } else {
    reply.httpReturn(404, "Wrong recording number!");
  }
}

/* move or copy recording */
void RecordingsResponder::moveRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
   QueryHandler q("/recordings/move", request);
   string source = q.getBodyAsString("source");
   string target = q.getBodyAsString("target");
   bool copy_only = q.getBodyAsBool("copy_only");


#if APIVERSNUM > 20300
    LOCK_RECORDINGS_READ;
    const cRecordings& recordings = *Recordings;
#else
    cThreadLock RecordingsLock(&Recordings);
    cRecordings& recordings = Recordings;
#endif

   if (source.length() <= 0 || target.length() <= 0) {
      reply.httpReturn(404, "Missing file name!");
      return;
   } else if (access(source.c_str(), F_OK) != 0) {
      reply.httpReturn(504, "Path is invalid!");
      return;
   }

   const cRecording* recording = recordings.GetByName(source.c_str());
   if (!recording) {
      reply.httpReturn(504, "Recording not found!");
      return;
   }

   string newname = VdrExtension::MoveRecording(recording, VdrExtension::FileSystemExchangeChars(target.c_str(), true), copy_only);

   if (newname.length() <= 0) {
      LOG_ERROR_STR(source.c_str());
      reply.httpReturn(503, "File copy failed!");
      return;
   }

   const cRecording* new_recording = recordings.GetByName(newname.c_str());
   if (!new_recording) {
      LOG_ERROR_STR(newname.c_str());
      reply.httpReturn(504, "Recording not found, after moving!");
      return;
   }


   esyslog("restfulapi: %s, %d", new_recording->FileName(), new_recording->Index());

   RecordingList* recordingList = getRecordingList(out, q, reply);
   recordingList->addRecording(new_recording, new_recording->Index(), NULL, "");
   recordingList->setTotal(recordings.Count());
   recordingList->finish();
   delete recordingList;
}

void RecordingsResponder::replyEditedFileName(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/recordings/editedfile", request);
  const cRecording* recording		= getRecordingByRequest(q);
  RecordingList* recordingList	= getRecordingList(out, q, reply);
  const cRecording* editedFile	= NULL;

  if ( recordingList == NULL ) {

      return;
  };
  if ( recording == NULL ) {

      reply.httpReturn(404, "Requested recording not found!");
      return;
  }

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_READ;
    const cRecordings& recordings = *Recordings;
#else
    cRecordings& recordings = Recordings;
#endif
  editedFile = recordings.GetByName(cCutter::EditedFileName(recording->FileName()));
  if ( editedFile == NULL ) {

      reply.httpReturn(404, "Requested edited file not found!");
      return;
  };

  recordingList->init();
  recordingList->addRecording(editedFile, editedFile->Index(), NULL, "");
  recordingList->setTotal(recordings.Count());
  recordingList->finish();
  delete recordingList;

};

void RecordingsResponder::deleteRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings", request);
  cRecording* delRecording = getRecordingByRequestWrite(q);
  string syncId = q.getOptionAsString("syncId");

  if ( delRecording == NULL ) {
      reply.httpReturn(404, "Recording not found!");
      return;
  }

  esyslog("restfulapi: delete recording %s", delRecording->FileName());
  if ( delRecording->Delete() ) {

    if (syncId != "") {
      SyncMap* syncMap = new SyncMap(q, true);
      syncMap->erase(StringExtension::toString(delRecording->FileName()));
    }

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_WRITE;
    cRecordings& recordings = *Recordings;
#else
    cRecordings& recordings = Recordings;
#endif
    recordings.DelByName(delRecording->FileName());
    reply.httpReturn(200, "Recording deleted!");
    return;
  }
  reply.httpReturn(500, "Recording could not be deleted!");
}

void RecordingsResponder::showRecordings(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings", request);
  bool read_marks = q.getOptionAsString("marks") == "true";
  string sync_id = q.getOptionAsString("syncId");
  SyncMap* sync_map = new SyncMap(q);

  RecordingList* recordingList = getRecordingList(out, q, reply, read_marks);

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  
  const cRecording* recording = getRecordingByRequest(q);

  if ( start_filter >= 0 && limit_filter >= 1 ) {
     recordingList->activateLimit(start_filter, limit_filter);
  }

  recordingList->init();

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_READ;
    const cRecordings& recordings = *Recordings;
#else
    cRecordings& recordings = Recordings;
#endif

  if ( recording == NULL ) {
     for (int i = 0; i < recordings.Count(); i++) {
        recordingList->addRecording(recordings.Get(i), i, sync_map, "");
     }
  } else {
     recordingList->addRecording(recording, recording->Index(), sync_map, "");
  }
  recordingList->setTotal(recordings.Count());

  recordingList->finish();
  delete recordingList;

  if (sync_map->active()) {
      sync_map->write();
  }
  delete sync_map;
}

void RecordingsResponder::saveMarks(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/marks", request);
  const cRecording* recording = getRecordingByRequest(q);
  JsonArray* jsonArray = q.getBodyAsArray("marks");

  if (jsonArray == NULL) {
     reply.httpReturn(503, "Marks in HTTP-Body are missing.");
  } else {

    if ( recording  == NULL ) {
       reply.httpReturn(504, "Recording number/name missing or invalid.");
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

       VdrMarks::get()->saveMarks(recording, marks);
    }
  }
}

void RecordingsResponder::deleteMarks(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/marks", request);
  const cRecording* recording = getRecordingByRequest(q);
  if ( recording != NULL ) {
     if (VdrMarks::get()->deleteMarks(recording)) {
        return;
     }
  }
  reply.httpReturn(503, "Deleting marks failed.");
}

void RecordingsResponder::cutRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/cut", request);
  const cRecording* recording = getRecordingByRequest(q);

  if ( recording != NULL ) {
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

void RecordingsResponder::replyUpdates(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

	QueryHandler q("/recordings/updates", request);
	SyncMap* sync_map = new SyncMap(q);

	if (sync_map->active()) {
		this->initServerList(out, request, reply, sync_map);
		this->sendSyncList(out, request, reply, sync_map);
	}
};

void RecordingsResponder::replySyncList(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

	QueryHandler q("/recordings/sync", request);
	map<string, string> clientMap;
	map<string, string>::iterator it;
	vector<string> clientList = q.getBodyAsStringArray("recordings");
	SyncMap* sync_map = new SyncMap(q);

	if (sync_map->active()) {

		this->initServerList(out, request, reply, sync_map);

		for (unsigned i=0; i < clientList.size(); i++) {

			string s = clientList[i];
			string key = s.substr(0, s.find_last_of(","));
			string value = s.substr(s.find_last_of(",") + 1);
			clientMap[key] = value;
			//esyslog("restfulapi: %s : %s", key.c_str(), value.c_str());
		}
		sync_map->setClientMap(clientMap);
		this->sendSyncList(out, request, reply, sync_map);
	}
};

void RecordingsResponder::initServerList(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, SyncMap* sync_map) {

	QueryHandler q("/recordings/sync", request);
	RecordingList* recordingList = getRecordingList(out, q, reply, false);

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_READ;
    const cRecordings& recordings = *Recordings;
#else
    cRecordings& recordings = Recordings;
#endif
	for (int i = 0; i < recordings.Count(); i++) {
		recordingList->addRecording(recordings.Get(i), i, sync_map, "");
	}
	delete recordingList;
};

void RecordingsResponder::sendSyncList(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, SyncMap* sync_map) {

	QueryHandler q("/recordings/sync", request);
	RecordingList* updates = getRecordingList(out, q, reply, false);
	map<string, string> updatesList;
	map<string, string>::iterator itUpdates;

	updatesList = sync_map->getUpdates();
	updates->init();

	esyslog("restfulapi: recording updates: %d", (int)updatesList.size());

#if APIVERSNUM > 20300
    LOCK_RECORDINGS_READ;
    const cRecordings& recordings = *Recordings;
#else
    cRecordings& recordings = Recordings;
#endif

	for (itUpdates = updatesList.begin(); itUpdates != updatesList.end(); itUpdates++) {

		if ( "delete" != itUpdates->second) {
			const cRecording* recording = recordings.GetByName(itUpdates->first.c_str());
			updates->addRecording(recording, recording->Index(), NULL, itUpdates->second, true);
		} else {
			const cRecording* recording = new cRecording(itUpdates->first.c_str());
			updates->addRecording(recording, -1, NULL, itUpdates->second, true);
			delete recording;
		}
	}
	updates->setTotal(updatesList.size());
	updates->finish();
	delete updates;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p)
{
  si.addMember("number") <<= p.Number;
  si.addMember("name") <<= p.Name;
  si.addMember("file_name") <<= p.FileName;
  si.addMember("relative_file_name") <<= p.RelativeFileName;
  si.addMember("inode") <<= p.Inode;
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
  si.addMember("additional_media") <<= p.AdditionalMedia;
  si.addMember("aux") <<= p.Aux;
  si.addMember("sync_action") <<= p.SyncAction;
  si.addMember("hash") <<= p.hash;
}

RecordingList::RecordingList(ostream *out, bool _read_marks)
{
  s = new StreamExtension(out);
  read_marks = _read_marks;
  Scraper2VdrService sc;
  total = 0;
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

void HtmlRecordingList::addRecording(const cRecording* recording, int nr, SyncMap* sync_map, string sync_action, bool add_hash)
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

void JsonRecordingList::addRecording(const cRecording* recording, int nr, SyncMap* sync_map, string sync_action, bool add_hash)
{
  if ( filtered() ) return;

  cxxtools::String empty = StringExtension::UTF8Decode("");
  cxxtools::String eventTitle = empty;
  cxxtools::String eventShortText = empty;
  cxxtools::String eventDescription = empty;
  cxxtools::String hash = empty;
  int eventStartTime = -1;
  int eventDuration = -1;
    
  cEvent* event = (cEvent*)recording->Info()->GetEvent();
  
  if (event != NULL)
  {
     if (event->Title())         { eventTitle = StringExtension::UTF8Decode(event->Title()); }
     if (event->ShortText())     { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
     if (event->Description())   { eventDescription = StringExtension::UTF8Decode(event->Description()); }
     if (event->StartTime() > 0) { eventStartTime = event->StartTime(); }
     if (event->Duration() > 0)  { eventDuration = event->Duration(); }
  }

  SerRecording serRecording;

  SerAdditionalMedia am;
  if (sc.getMedia(recording, am)) {
      serRecording.AdditionalMedia = am;
  }

  serRecording.Number = nr;
  serRecording.Name = StringExtension::encodeToJson(recording->Name());
  serRecording.FileName = StringExtension::UTF8Decode(recording->FileName());
  serRecording.RelativeFileName = StringExtension::UTF8Decode(VdrExtension::getRelativeVideoPath(recording).c_str());
  serRecording.IsNew = recording->IsNew();
  serRecording.IsEdited = recording->IsEdited();

  const char* filename = recording->FileName();
  struct stat st;
  if (stat(filename, &st) == 0) {
      serRecording.Inode = StringExtension::encodeToJson((const char*)cString::sprintf("%lu:%llu", (unsigned long) st.st_dev, (unsigned long long) st.st_ino));
  }

  serRecording.IsPesRecording = recording->IsPesRecording();
  serRecording.FramesPerSecond = recording->FramesPerSecond();

  serRecording.Duration = VdrExtension::RecordingLengthInSeconds(recording);
  serRecording.FileSizeMB = recording->FileSizeMB();
  serRecording.ChannelID = StringExtension::UTF8Decode((string) recording->Info()->ChannelID().ToString());

  serRecording.EventTitle = eventTitle;
  serRecording.EventShortText = eventShortText;
  serRecording.EventDescription = eventDescription;
  serRecording.EventStartTime = eventStartTime;
  serRecording.EventDuration = eventDuration;

  const char* aux = recording->Info()->Aux();
  serRecording.Aux = (aux != NULL ? StringExtension::UTF8Decode(aux) : empty);

  SerMarks serMarks;
  if (read_marks) {
     serMarks.marks = VdrMarks::get()->readMarks(recording);
  }
  serRecording.Marks = serMarks;
  serRecording.SyncAction = (cxxtools::String)sync_action;

  if ( add_hash == true || ( sync_map != NULL && sync_map->active() ) ) {
	  hash = cxxtools::md5(cxxtools::JsonSerializer::toString(serRecording, "recording"));
  }
  if ( sync_map != NULL && sync_map->active() ) {
	  sync_map->add((string)filename, StringExtension::toString(hash));
  }

  serRecording.hash = hash;

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

void XmlRecordingList::addRecording(const cRecording* recording, int nr, SyncMap* sync_map, string sync_action, bool add_hash)
{
  if ( filtered() ) return;

  string eventTitle = "";
  string eventShortText = "";
  string eventDescription = "";
  string hash = "";
  int eventStartTime = -1;
  int eventDuration = -1;

  string out;

  cEvent* event = (cEvent*)recording->Info()->GetEvent();

  if (event != NULL)
  {
     if (event->Title())			{ eventTitle		= event->Title();		}
     if (event->ShortText())		{ eventShortText	= event->ShortText();	}
     if (event->Description())		{ eventDescription	= event->Description();	}
     if (event->StartTime() > 0)	{ eventStartTime	= event->StartTime();	}
     if (event->Duration() > 0)		{ eventDuration		= event->Duration();	}
  }

  out = " <recording>\n";
  out += cString::sprintf("  <param name=\"number\">%i</param>\n", nr);
  out += cString::sprintf("  <param name=\"name\">%s</param>\n", StringExtension::encodeToXml(recording->Name()).c_str() );
  out += cString::sprintf("  <param name=\"filename\">%s</param>\n", StringExtension::encodeToXml(recording->FileName()).c_str());
  out += cString::sprintf("  <param name=\"relative_filename\">%s</param>\n", StringExtension::encodeToXml(VdrExtension::getRelativeVideoPath(recording).c_str()).c_str());

  const char* filename = recording->FileName();
  struct stat st;
  if (stat(filename, &st) == 0) {

      out += cString::sprintf(
    		  "  <param name=\"inode\">%s</param>\n",
			  StringExtension::encodeToXml(
					  (const char*)cString::sprintf("%lu:%llu", (unsigned long) st.st_dev, (unsigned long long) st.st_ino)
			  ).c_str()
      );
  }

  out += cString::sprintf("  <param name=\"is_new\">%s</param>\n", recording->IsNew() ? "true" : "false" );
  out += cString::sprintf("  <param name=\"is_edited\">%s</param>\n", recording->IsEdited() ? "true" : "false" );
  out += cString::sprintf("  <param name=\"is_pes_recording\">%s</param>\n", recording->IsPesRecording() ? "true" : "false" );
  out += cString::sprintf("  <param name=\"frames_per_second\">%.2f</param>\n", recording->FramesPerSecond());
  out += cString::sprintf("  <param name=\"duration\">%i</param>\n", VdrExtension::RecordingLengthInSeconds(recording));
  out += cString::sprintf("  <param name=\"filesize_mb\">%i</param>\n", recording->FileSizeMB());
  out += cString::sprintf("  <param name=\"channel_id\">%s</param>\n", StringExtension::encodeToXml((string) recording->Info()->ChannelID().ToString()).c_str());

  if (read_marks) {
     out += "  <param name=\"marks\">\n";
     vector< string > marks = VdrMarks::get()->readMarks(recording);
     for(int i=0;i<(int)marks.size();i++) {
	 out += cString::sprintf("   <mark>%s</mark>\n", marks[i].c_str());
     }
     out += "  </param>\n";
  }

  out += cString::sprintf("  <param name=\"event_title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str());
  out += cString::sprintf("  <param name=\"event_short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str());
  out += cString::sprintf("  <param name=\"event_description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str());
  out += cString::sprintf("  <param name=\"event_start_time\">%i</param>\n", eventStartTime);
  out += cString::sprintf("  <param name=\"event_duration\">%i</param>\n", eventDuration);

  const char* aux = recording->Info()->Aux();
  out += cString::sprintf("  <param name=\"aux\">%s</param>\n", StringExtension::encodeToXml((aux != NULL ? aux : "")).c_str());
  out += sc.getMedia(recording);
  out += cString::sprintf("  <param name=\"sync_action\">%s</param>\n", StringExtension::encodeToXml(sync_action).c_str());

  if ( add_hash == true || ( sync_map != NULL && sync_map->active() ) ) {
	  hash = (string)cxxtools::md5(out);
  }
  if ( sync_map != NULL && sync_map->active() ) {
      sync_map->add((string)filename, hash);
  }

  out += cString::sprintf("  <param name=\"hash\">%s</param>\n", StringExtension::encodeToXml(hash).c_str());
  out += " </recording>\n";

  s->write(out.c_str());
}

void XmlRecordingList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</recordings>");
}

// Class SyncMap

SyncMap::SyncMap(QueryHandler q, bool overrideFormat) {

  string sync_id = q.getOptionAsString("syncId");
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");

  if ( start_filter < 0 && limit_filter < 0 && sync_id != "" && ( overrideFormat || q.isFormat(".json") || q.isFormat(".xml") ) ) {
      this->id = sync_id;
  } else {
      this->id = "";
  }
};

/**
 * check whether sync map is active (for writing to disk) or not
 */
bool SyncMap::active() {

  return this->id != "";
};

/**
 * retrieve pointer to sync file
 */
FILE* SyncMap::getSyncFile(bool write = false) {

  string cacheDir = (string)Settings::get()->CacheDirectory() + "/sync";
  FileExtension::get()->exists(cacheDir) || system(("mkdir -p " + cacheDir).c_str());
  std::string fileName = (cacheDir + "/" + this->id);
  FILE* fp = fopen(fileName.c_str(), write ? "w" : "r");

  if ( fp == NULL ) {
      esyslog("restfulapi: could not open sync map file %s for %s!", fileName.c_str(), write ? "writing" : "reading");
  }

  return fp;
};

/**
 * add recording to sync list
 */
void SyncMap::add(string filename, string hash) {
  this->serverMap[filename] = hash;
};

/**
 * delete recording from sync list
 */
void SyncMap::erase(string filename) {

  esyslog("restfulapi: sync_map: erase %s", filename.c_str());
  this->load();
  map<string, string>::iterator it = this->clientMap.find(filename);
  if (it != this->clientMap.end()) {
      this->clientMap.erase(filename);
      this->write(false);
  }
};

/**
 * write sync file
 */
void SyncMap::write(bool server) {

  map<string, string> writeMap = server ? this->serverMap : this->clientMap;
  map<string, string>::iterator it;
  FILE* fp = this->getSyncFile(true);

  if ( fp != NULL ) {
    for(it = writeMap.begin(); it != writeMap.end(); it++) {
	fputs((it->first + "," + StringExtension::trim(it->second) + "\n").c_str(), fp);
    }

    fclose(fp);
    esyslog(
	"restfulapi: sync map file '%s' written with %d entries",
        ((string)Settings::get()->CacheDirectory() + "/sync/" + this->id).c_str(),
        (int)writeMap.size()
    );
  }
};

/**
 * clear requested list
 */
void SyncMap::clear(bool server) {

  map<string, string> clearMap = server ? this->serverMap : this->clientMap;
  clearMap.clear();
};

/**
 * load sync file
 */
void SyncMap::load() {

	this->clear(false);
	char line[1024];
	string recording;
	string filename;
	string hash;
	FILE* fp = this->getSyncFile();

	if ( fp != NULL ) {
		while (fgets(line, 1024, fp)) {

			recording = (string)line;
			filename = recording.substr(0, recording.find_last_of(","));
			hash = recording.substr(recording.find_last_of(",") + 1, recording.length() - 1);

			this->clientMap[filename] = hash;
		}

		fclose(fp);
	}
};

void SyncMap::setClientMap(map<string, string> clientMap) {

	this->clear(false);
	map<string, string>::iterator it;

	for (it = clientMap.begin(); it != clientMap.end(); it++) {

		this->clientMap[it->first] = it->second;
	}
};

/**
 * fill updatesList
 */
map<string, string> SyncMap::getUpdates() {

  this->load();
  map<string, string> updatesList;
  map<string, string>::iterator itServer;
  map<string, string>::iterator itClient;
  map<string, string>::iterator result;
  string hash;
  string fileName;
  string action;

  for (itServer = this->serverMap.begin(); itServer != this->serverMap.end(); itServer++) {
      fileName = itServer->first;
      hash = itServer->second;
      action = "";

      result = this->clientMap.find(fileName);
      if ( result == this->clientMap.end() ) {
    	  action = "add";
      } else if ( StringExtension::trim(result->second) != hash ) {
    	  action = "update";
      }

      if ( action != "" ) updatesList[fileName] = action;
  }

  for (itClient = this->clientMap.begin(); itClient != this->clientMap.end(); itClient++) {
      fileName = itClient->first;
      hash = itClient->second;
      action = "";

      result = this->serverMap.find(fileName);
      if ( result == this->serverMap.end() ) {
    	  action = "delete";
      }

      if ( action != "" ) updatesList[fileName] = action;
  }

  this->write();
  return updatesList;
};

void SyncMap::log(bool server) {

  map<string, string> logMap = server ? this->serverMap : this->clientMap;
  esyslog("restfulapi: ==== sync items: %d", (int)logMap.size());
  map<string, string>::iterator it;
  for (it = logMap.begin(); it != logMap.end(); it++) {

      esyslog("restfulapi: sync_map: '%s' => '%s'", it->first.c_str(), it->second.c_str());
  }
};








