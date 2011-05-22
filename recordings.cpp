#include "recordings.h"

void RecordingsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( request.method() == "GET" ) {
     showRecordings(out, request, reply);
  } else if ( request.method() == "DELETE" ) {
     deleteRecording(out, request, reply);
  } else {
     reply.httpReturn(501, "Only GET and DELETE methods are supported.");
  }
}

void RecordingsResponder::deleteRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/recordings", request.url());
  int recording_number = getIntParam(params, 0);
  if ( recording_number <= 0 || recording_number > Recordings.Count() ) { 
     reply.httpReturn(404, "Wrong recording number!");
  } else {
     recording_number--; // first recording is 0 and not 1 like in param
     cRecording* delRecording = Recordings.Get(recording_number);
     if ( delRecording->Delete() ) {
        Recordings.DelByName(delRecording->FileName());
     }
  }
}

void RecordingsResponder::showRecordings(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/recordings", request.url()); 
  RecordingList* recordingList;

  if ( isFormat(params, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     recordingList = (RecordingList*)new JsonRecordingList(&out);
  } else if ( isFormat(params, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     recordingList = (RecordingList*)new HtmlRecordingList(&out);
  } else {
     reply.httpReturn(404, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  recordingList->init();

  for (cRecording* recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
     recordingList->addRecording(recording); 
  }

  recordingList->finish();
  delete recordingList; 
}

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p)
{
  si.addMember("name") <<= p.Name;
  si.addMember("file_name") <<= p.FileName;
  si.addMember("is_new") <<= p.IsNew;
  si.addMember("is_edited") <<= p.IsEdited;
  si.addMember("is_pes_recording") <<= p.IsPesRecording;
  si.addMember("size") <<= p.Size;
  si.addMember("event_title") <<= p.EventTitle;
  si.addMember("event_short_text") <<= p.EventShortText;
  si.addMember("event_description") <<= p.EventDescription;
  si.addMember("event_start_time") <<= p.EventStartTime;
  si.addMember("event_duration") <<= p.EventDuration;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerRecordings& p)
{
  si.addMember("rows") <<= p.recording;
}

void HtmlRecordingList::init()
{
  writeHtmlHeader(out);
  
  write(out, "<ul>");
}

void HtmlRecordingList::addRecording(cRecording* recording)
{
  write(out, "<li>");
  write(out, (char*)recording->Name());
  write(out, "</body></html>");
}

void HtmlRecordingList::finish()
{
  write(out, "</ul>");
  write(out, "</body></html>");
}

void JsonRecordingList::addRecording(cRecording* recording)
{
  cxxtools::String empty = UTF8Decode("");
  cxxtools::String eventTitle = empty;
  cxxtools::String eventShortText = empty;
  cxxtools::String eventDescription = empty;
  int eventStartTime = -1;
  int eventDuration = -1;

  cEvent* event = (cEvent*)recording->Info()->GetEvent();

  if ( event != NULL )
  {
     if ( event->Title() ) { eventTitle = UTF8Decode(event->Title()); }
     if ( event->ShortText() ) { eventShortText = UTF8Decode(event->ShortText()); }
     if ( event->Description() ) { eventDescription = UTF8Decode(event->Description()); }
     if ( event->StartTime() > 0 ) { eventStartTime = event->StartTime(); }
     if ( event->Duration() > 0 ) { eventDuration = event->Duration(); }
  }

  int size = getSizeOfRecording((std::string)recording->FileName());

  SerRecording serRecording;
  serRecording.Name = UTF8Decode(recording->Name());
  serRecording.FileName = UTF8Decode(recording->FileName());
  serRecording.IsNew = recording->IsNew();
  serRecording.IsEdited = recording->IsEdited();
  serRecording.IsPesRecording = recording->IsPesRecording();
  serRecording.Size = size;
  serRecording.EventTitle = eventTitle;
  serRecording.EventShortText = eventShortText;
  serRecording.EventDescription = eventDescription;
  serRecording.EventStartTime = eventStartTime;
  serRecording.EventDuration = eventDuration;
  serRecordings.push_back(serRecording);
}

void JsonRecordingList::finish()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serRecordings, "recordings");
  serializer.finish();
}
