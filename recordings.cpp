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
     if ( request.method() == "GET" ) {
        showCutterStatus(out, request, reply);
     } else if (request.method() == "POST") {
        cutRecording(out, request, reply); 
     } else {
        reply.httpReturn(501, "Only GET and POST methods are supported by the /recordings/cut service.");
     }
     found = true;
  }

  else if ((int)request.url().find("/recordings/marks") == 0 ) {
     if ( request.method() == "DELETE" ) {
        deleteMarks(out, request, reply);
     } else if (request.method() == "POST" ) {
        saveMarks(out, request, reply);
     } else {
        reply.httpReturn(501, "Only DELETE and POST methods are supported by the /recordings/marks service.");
     }
     found = true;
  }

  // original /recordings service
  else if ((int) request.url().find("/recordings") == 0 ) {
        if ( request.method() == "GET" ) {
        showRecordings(out, request, reply);
        found = true;
     } else if (request.method() == "DELETE" ) {
        deleteRecording(out, request,reply);
        found = true;
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
  } else if ( q.isFormat(".xml") )  {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     recordingList = (RecordingList*)new XmlRecordingList(&out, read_marks);
  } else {
     reply.httpReturn(404, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  
  int requested_item = q.getParamAsInt(0);

  if ( start_filter >= 0 && limit_filter >= 1 ) {
     recordingList->activateLimit(start_filter, limit_filter);
  }

  recordingList->init();

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

  cEvent* event = (cEvent*)recording->Info()->GetEvent();

  if ( event != NULL )
  {
     if ( event->Title() ) { eventTitle = StringExtension::UTF8Decode(event->Title()); }
     if ( event->ShortText() ) { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
     if ( event->Description() ) { eventDescription = StringExtension::UTF8Decode(event->Description()); }
     if ( event->StartTime() > 0 ) { eventStartTime = event->StartTime(); }
     if ( event->Duration() > 0 ) { eventDuration = event->Duration(); }
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

  cEvent* event = (cEvent*)recording->Info()->GetEvent();

  if ( event != NULL )
  {
     if ( event->Title() ) { eventTitle = event->Title(); }
     if ( event->ShortText() ) { eventShortText = event->ShortText(); }
     if ( event->Description() ) { eventDescription = event->Description(); }
     if ( event->StartTime() > 0 ) { eventStartTime = event->StartTime(); }
     if ( event->Duration() > 0 ) { eventDuration = event->Duration(); }
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

  s->write(cString::sprintf("  <param name=\"event_title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str()) );
  s->write(cString::sprintf("  <param name=\"event_short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str()) );
  s->write(cString::sprintf("  <param name=\"event_description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str()) );
  s->write(cString::sprintf("  <param name=\"event_start_time\">%i</param>\n", eventStartTime));
  s->write(cString::sprintf("  <param name=\"event_duration\">%i</param>\n", eventDuration));
  s->write(" </recording>\n");
}

void XmlRecordingList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</recordings>");
}
