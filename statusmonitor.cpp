#include "statusmonitor.h"

// --- TextOsd ---------------------------------------------------------------------------------------

TextOsd::TextOsd()
{
  _selected = NULL;
}

TextOsd::~TextOsd()
{
  ClearItems();
}

OsdItem* TextOsd::GetItem(int i)
{
  std::list<OsdItem*>::iterator it;
  int counter = 0;

  for (it=_items.begin(); it != _items.end(); it++)
  {
    if ( counter == i ) return *it;
    counter++;
  }

  return NULL;
}

int TextOsd::CountItems()
{
  return _items.size();
}

void TextOsd::ClearItems()
{
  if (_selected != NULL)
     delete _selected;
  _selected = NULL;

  while(_items.size() != 0 ) {
     OsdItem* swap = _items.back();
     _items.pop_back();
     delete swap;
  }
}

void TextOsd::RemoveItem(OsdItem* item)
{
  _items.remove(item);
}

void TextOsd::RemoveItem(std::string item)
{
  OsdItem* o = NULL;
  std::list<OsdItem*>::iterator it;
  for(it = _items.begin(); it != _items.end(); it++) {
     if ((*it)->Text() == item)
        o = *it;
  }
  if ( o != NULL ) {
     _items.remove(o);
  }
}

std::string TextOsd::Title() { return _title; }
std::string TextOsd::Message() { return _message; }
std::string TextOsd::Text() { return _text; }
std::string TextOsd::Red() { return _red; }
std::string TextOsd::Green() { return _green; }
std::string TextOsd::Yellow() { return _yellow; }
std::string TextOsd::Blue() { return _blue; }
OsdItem*    TextOsd::Selected() { return _selected; }

void TextOsd::Title(const char* title) { _title = std::string(title); }
void TextOsd::Message(const char* message) { _message = std::string(message); }
void TextOsd::Text(const char* text) { _text = std::string(text); }
void TextOsd::Red(const char* red) { _red = std::string(red); }
void TextOsd::Green(const char* green) { _green = std::string(green); }
void TextOsd::Yellow(const char* yellow) { _yellow = std::string(yellow); }
void TextOsd::Blue(const char* blue) { _blue = std::string(blue); }
void TextOsd::Selected(OsdItem* item) { _selected = item; }

// --- StatusMonitor ---------------------------------------------------------------------------------

void StatusMonitor::OsdCreate(void)
{
  if ( _osd == NULL )
     _osd = new TextOsd();
}

void StatusMonitor::TimerChange(const cTimer *Timer, eTimerChange Change)
{

}

void StatusMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  device = (cDevice*)Device;
  channel_number = ChannelNumber;
}

void StatusMonitor::Recording(const cDevice *Device, const char *Name, const char *FileName, bool On)
{

}

void StatusMonitor::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
  recording_name = std::string(Name);
  recording_file = std::string(FileName);
}

void StatusMonitor::SetVolume(int Volume, bool Absolute)
{
  if(Absolute) {
    volume = Volume;
  } else {
    volume += Volume;
  }
}

void StatusMonitor::SetAudioTrack(int Index, const char * const *Tracks)
{

}

void StatusMonitor::SetAudioChannel(int AudioChannel)
{

}

void StatusMonitor::SetSubtitleTrack(int Index, const char * const *Tracks)
{

}

void StatusMonitor::OsdClear(void)
{
  if ( _osd != NULL ) 
     delete _osd;
  _osd = NULL;  
}

void StatusMonitor::OsdTitle(const char *Title)
{
  OsdCreate(); 
  _osd->Title(Title);
}

void StatusMonitor::OsdStatusMessage(const char *Message)
{
  OsdCreate();
  _osd->Message(Message);
}

void StatusMonitor::OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  OsdCreate();
  _osd->Red(Red);
  _osd->Green(Green);
  _osd->Yellow(Yellow);
  _osd->Blue(Blue);
}

void StatusMonitor::OsdItem(const char *Text, int Index)
{
  OsdCreate();
  //add items
}

void StatusMonitor::OsdCurrentItem(const char *Text)
{
  OsdCreate();
  //update current item
}

void StatusMonitor::OsdTextItem(const char *Text, bool Scroll)
{
  OsdCreate();
  _osd->Text(Text);
}

void StatusMonitor::OsdChannel(const char *Text)
{

}

void StatusMonitor::OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle)
{

}


