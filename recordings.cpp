#include "recordings.h"

void RecordingsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string qparams = request.qparams();
  RecordingList* recordingList;

  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  if ( isFormat(qparams, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     recordingList = (RecordingList*)new JsonRecordingList(&out);
  } else if ( isFormat(qparams, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     recordingList = (RecordingList*)new HtmlRecordingList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  for (cRecording* recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
     recordingList->addRecording(recording); 
  }
 
  if ( isFormat(qparams, ".json") ) {
     delete (JsonRecordingList*)recordingList;
  } else {
     delete (HtmlRecordingList*)recordingList;
  } 
}

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p)
{
  si.addMember("name") <<= p.Name;
  si.addMember("file_name") <<= p.FileName;
  si.addMember("is_new") <<= p.IsNew;
  si.addMember("is_edited") <<= p.IsEdited;
  si.addMember("is_pes_recording") <<= p.IsPesRecording;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerRecordings& p)
{
  si.addMember("rows") <<= p.recording;
}

HtmlRecordingList::HtmlRecordingList(std::ostream* _out) : RecordingList(_out)
{
  writeHtmlHeader(out);
  
  write(out, "<ul>");
}

HtmlRecordingList::~HtmlRecordingList()
{
  write(out, "</ul>");
  write(out, "</body></html>");
}

void HtmlRecordingList::addRecording(cRecording* recording)
{
  write(out, "<li>");
  write(out, (char*)recording->Name());
  write(out, "</body></html>");
}

JsonRecordingList::JsonRecordingList(std::ostream* _out) : RecordingList(_out)
{

}

JsonRecordingList::~JsonRecordingList()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serRecordings, "recordings");
  serializer.finish();
}

void JsonRecordingList::addRecording(cRecording* recording)
{
  SerRecording serRecording;
  serRecording.Name = UTF8Decode(recording->Name());
  serRecording.FileName = UTF8Decode(recording->FileName());
  serRecording.IsNew = recording->IsNew();
  serRecording.IsEdited = recording->IsEdited();
  serRecording.IsPesRecording = recording->IsPesRecording();
  serRecordings.push_back(serRecording);
}
