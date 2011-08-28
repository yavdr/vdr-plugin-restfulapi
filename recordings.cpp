#include "recordings.h"

void RecordingsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  if ( request.method() == "GET" ) {
     showRecordings(out, request, reply);
  } else if ( request.method() == "DELETE" ) {
     if ( (int)request.url().find("/recordings/marks") != 0) {
        deleteMarks(out, request, reply);      
     } else {
        deleteRecording(out, request, reply);
     }
  } else if ((int)request.url().find("/recordings/marks") == 0 && request.method() == "POST") {
     saveMarks(out, request, reply);
  } else {
     reply.httpReturn(501, "Only GET, POST and DELETE methods are supported on these services.");
  }
}

void RecordingsResponder::deleteRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings", request);
  int recording_number = q.getParamAsInt(0);
  if ( recording_number < 0 || recording_number >= Recordings.Count() ) { 
     reply.httpReturn(404, "Wrong recording number!");
  } else {
     cRecording* delRecording = Recordings.Get(recording_number);
     if ( delRecording->Delete() ) {
        Recordings.DelByName(delRecording->FileName());
     }
  }
}

void RecordingsResponder::showRecordings(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
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
  
  cRecording* recording = NULL;
  for (int i = 0; i < Recordings.Count();i++) {
     if ( requested_item == i || requested_item < 0 ) {
        recording = Recordings.Get(i);
        recordingList->addRecording(recording, i); 
     }
  }
  recordingList->setTotal(Recordings.Count());

  recordingList->finish();
  delete recordingList;
}

void RecordingsResponder::saveMarks(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/marks", request);
  int recording = q.getParamAsInt(0);
  JsonArray* jsonArray = q.getBodyAsArray("marks");

  if (jsonArray == NULL) {
     reply.httpReturn(503, "Marks in HTTP-Body are missing.");
  }

  if (recording < 0 && recording >= Recordings.Count()) {
     reply.httpReturn(504, "Recording number missing or invalid.");
  }

  std::vector< std::string > marks;

  for(int i=0;i<jsonArray->CountItem();i++) {
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

void RecordingsResponder::deleteMarks(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/marks", request);
  int rec_number = q.getParamAsInt(0);
  if (rec_number >= 0 && rec_number < Recordings.Count()) {
     cRecording* recording = Recordings.Get(rec_number);
     if (VdrMarks::get()->deleteMarks(recording)) {
        return;
     }
  }
  reply.httpReturn(503, "Deleting marks failed.");
}

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p)
{
  si.addMember("number") <<= p.Number;
  si.addMember("name") <<= p.Name;
  si.addMember("file_name") <<= p.FileName;
  si.addMember("is_new") <<= p.IsNew;
  si.addMember("is_edited") <<= p.IsEdited;
  si.addMember("is_pes_recording") <<= p.IsPesRecording;
  si.addMember("duration") <<= p.Duration;
  si.addMember("frames_per_second") <<= p.FramesPerSecond;
  si.addMember("marks") <<= p.Marks.marks;
  si.addMember("event_title") <<= p.EventTitle;
  si.addMember("event_short_text") <<= p.EventShortText;
  si.addMember("event_description") <<= p.EventDescription;
  si.addMember("event_start_time") <<= p.EventStartTime;
  si.addMember("event_duration") <<= p.EventDuration;
}

RecordingList::RecordingList(std::ostream *out, bool _read_marks)
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
  serRecording.Name = StringExtension::UTF8Decode(recording->Name());
  serRecording.FileName = StringExtension::UTF8Decode(recording->FileName());
  serRecording.IsNew = recording->IsNew();
  serRecording.IsEdited = recording->IsEdited();
  serRecording.IsPesRecording = recording->IsPesRecording();
  serRecording.Duration = recording->LengthInSeconds() == -1 ? -1 : recording->LengthInSeconds()/60;
  serRecording.FramesPerSecond = recording->FramesPerSecond();
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

  std::string eventTitle = "";
  std::string eventShortText = "";
  std::string eventDescription = "";
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
  s->write(cString::sprintf("  <param name=\"name\">%s</param>\n", StringExtension::encodeToXml(recording->Name()).c_str()) );
  s->write(cString::sprintf("  <param name=\"filename\">%s</param>\n", StringExtension::encodeToXml(recording->FileName()).c_str()) );
  s->write(cString::sprintf("  <param name=\"is_new\">%s</param>\n", recording->IsNew() ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"is_edited\">%s</param>\n", recording->IsEdited() ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"is_pes_recording\">%s</param>\n", recording->IsPesRecording() ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"duration\">%i</param>\n", (recording->LengthInSeconds() == -1 ? -1 : recording->LengthInSeconds()/60)));
  s->write(cString::sprintf("  <param name=\"frames_per_second\">%f</param>\n", recording->FramesPerSecond()));

  if (read_marks) {
     s->write("  <param name=\"marks\">\n");
     std::vector< std::string > marks = VdrMarks::get()->readMarks(recording);
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
