#include "statusmonitor.h"

// --- TextOsd ---------------------------------------------------------------------------------------

TextOsd::TextOsd()
{
  _selected = NULL;
  _title = "";
  _message = "";
  _red = "";
  _green = "";
  _yellow = "";
  _blue = "";
}

TextOsd::~TextOsd()
{
  ClearItems();
}

TextOsdItem* TextOsd::GetItem(int i)
{
  std::list<TextOsdItem*>::iterator it;
  int counter = 0;

  for (it=_items.begin(); it != _items.end(); it++)
  {
    if ( counter == i ) return *it;
    counter++;
  }

  return NULL;
}

TextOsdItem* TextOsd::GetItem(std::string item)
{
  std::list<TextOsdItem*>::iterator it;
  for(it = _items.begin(); it != _items.end();++it)
  {
     if ((*it)->Text() == item)
     { 
        return *it;
     }
  }
  return NULL;
}

bool TextOsd::ReplaceItem(TextOsdItem* item, int i)
{
  if ((size_t)i < _items.size() && i >= 0)
  {
     TextOsdItem* old = NULL; 
     int counter = 0;
     std::list<TextOsdItem*>::iterator it;
     for(it = _items.begin();it != _items.end();++it)
     {
        if (counter == i)
        {
           old = *it;
           *it = item;
        }
        counter++;
     }
     if ( old != NULL ) {
        delete old;
     }
     return true;
  }
  return false;

}

int TextOsd::CountItems()
{
  return _items.size();
}

void TextOsd::ClearItems()
{
  _selected = NULL;

  while(_items.size() != 0 ) {
     TextOsdItem* swap = _items.back();
     _items.pop_back();
     delete swap;
  }
}

void TextOsd::AddItem(TextOsdItem* item)
{
  _items.push_back(item);
}

void TextOsd::RemoveItem(TextOsdItem* item)
{
  _items.remove(item);
}

void TextOsd::RemoveItem(std::string item)
{
  TextOsdItem* o = NULL;
  std::list<TextOsdItem*>::iterator it;
  for(it = _items.begin(); it != _items.end(); it++) {
     if ((*it)->Text() == item)
        o = *it;
  }
  if ( o != NULL ) {
     _items.remove(o);
  }
}

std::string  TextOsd::Title() { return _title; }
std::string  TextOsd::Message() { return _message; }
std::string  TextOsd::Red() { return _red; }
std::string  TextOsd::Green() { return _green; }
std::string  TextOsd::Yellow() { return _yellow; }
std::string  TextOsd::Blue() { return _blue; }
TextOsdItem* TextOsd::Selected() { return _selected; }

void TextOsd::Title(std::string title) { _title = title; }
void TextOsd::Message(std::string message) { _message = message; }
void TextOsd::Red(std::string red) { _red = red; }
void TextOsd::Green(std::string green) { _green = green; }
void TextOsd::Yellow(std::string yellow) { _yellow = yellow; }
void TextOsd::Blue(std::string blue) { _blue = blue; }
void TextOsd::Selected(TextOsdItem* item) { _selected = item; }

// --- ProgrammeOsd ----------------------------------------------------------------------------------

ProgrammeOsd::ProgrammeOsd(time_t PresentTime, std::string PresentTitle, std::string PresentSubtitle,
                           time_t FollowingTime, std::string FollowingTitle, std::string FollowingSubtitle)
{
  _presentTime = PresentTime;
  _presentTitle = PresentTitle;
  _presentSubtitle = PresentSubtitle;
  _followingTime = FollowingTime;
  _followingTitle = FollowingTitle;
  _followingSubtitle = FollowingSubtitle;
}

// --- StatusMonitor ---------------------------------------------------------------------------------

void StatusMonitor::OsdCreate(void)
{
  if ( _osd == NULL )
     _osd = (BasicOsd*)new TextOsd();
  if ( _osd != NULL && _osd->Type() != 0x01 ) {
     OsdDestroy();
     _osd = (BasicOsd*)new TextOsd();
  }
}

void StatusMonitor::OsdDestroy(void)
{
  //TaskScheduler currently buggy...
  //TaskScheduler::get()->AddTask((BaseTask*)new DeleteOsdTask(_osd));
  //wait until users got the data:
  if ( _osd != NULL ) {
     usleep(100); //wait 0.1ms
     if ( _osd->Type() == 0x01) {
        TextOsd* tosd = (TextOsd*)_osd;
        tosd->ClearItems();
     }
     delete _osd;
     _osd = NULL;
  }
}

void StatusMonitor::TimerChange(const cTimer *Timer, eTimerChange Change)
{
  //not important for restfulapi because of timer_server already being available?
}

#if APIVERSNUM >= 10726
void StatusMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView)
#else
void StatusMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber)
#endif
{
  if (ChannelNumber != 0) {
     channel_number = ChannelNumber;
  }
}

void StatusMonitor::Recording(const cDevice *Device, const char *Name, const char *FileName, bool On)
{
  //to be implemented
}

void StatusMonitor::Replaying(const cControl *Control, const char *Name, const char *FileName, bool On)
{
  if (On) {
     if(Name != NULL) recording_name = std::string(Name); else recording_name = "";
     if(FileName != NULL) recording_file = std::string(FileName); else recording_file = "";
  } else {
     recording_name = "";
     recording_file = "";
  }
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
  //to be implemented
}

void StatusMonitor::SetAudioChannel(int AudioChannel)
{
  //to be implemented
}

void StatusMonitor::SetSubtitleTrack(int Index, const char * const *Tracks)
{
  //to be implemented
}

void StatusMonitor::OsdClear(void)
{
  OsdDestroy();
}

void StatusMonitor::OsdTitle(const char *Title)
{
  OsdCreate(); 
  TextOsd* _tOsd = (TextOsd*)_osd;
  if (Title != NULL)
     _tOsd->Title((std::string)Title);
}

void StatusMonitor::OsdStatusMessage(const char *Message)
{
  OsdCreate();
  TextOsd* _tOsd = (TextOsd*)_osd;
  if (Message != NULL)
     _tOsd->Message((std::string)Message);
}

void StatusMonitor::OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{
  OsdCreate();
  TextOsd* _tOsd = (TextOsd*)_osd;
  if(Red != NULL) _tOsd->Red((std::string)Red);
  if(Green != NULL) _tOsd->Green((std::string)Green);
  if(Yellow != NULL) _tOsd->Yellow((std::string)Yellow);
  if(Blue != NULL) _tOsd->Blue((std::string)Blue);
}

void StatusMonitor::OsdItem(const char *Text, int Index)
{
  OsdCreate();
  TextOsd* _tOsd = (TextOsd*)_osd;
  if (Text != NULL)
  {
     bool result = _tOsd->ReplaceItem(new TextOsdItem((std::string)Text), Index);
     if (!result) {
        _tOsd->AddItem(new TextOsdItem((std::string)Text));
     }
  }
}

void StatusMonitor::OsdCurrentItem(const char *Text)
{
  if ( Text == NULL ) return;
  OsdCreate();
  TextOsd* _tOsd = (TextOsd*)_osd;

  TextOsdItem* osdItem = _tOsd->GetItem((std::string)Text);

  if ( osdItem == NULL && _tOsd->Selected() == NULL ) {
      TextOsdItem* newItem = new TextOsdItem((std::string)Text);
      _tOsd->AddItem(newItem);
  } else if ( osdItem == NULL) {
     _tOsd->Selected()->Text(Text);
  } else {
     _tOsd->Selected(osdItem);
  }
}

void StatusMonitor::OsdTextItem(const char *Text, bool Scroll)
{
  OsdCreate();
  TextOsd* _tOsd = (TextOsd*)_osd;
  if (Text != NULL)
  {
     //std::vector< std::string > lines = StringExtension::split((std::string)Text, "\n");
     //for(int i=0;i<(int)lines.size();i++)
        _tOsd->AddItem(new TextOsdItem((std::string)Text)/*new TextOsdItem(lines[i])*/);
  }
}

void StatusMonitor::OsdChannel(const char *Text)
{
  if ( Text != NULL ) {
     OsdDestroy();
     _osd = (BasicOsd*)new ChannelOsd((std::string)Text);
  }
}

void StatusMonitor::OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle)
{
  OsdDestroy();

  std::string presentTitle = "";
  std::string presentSubtitle = "";
  std::string followingTitle = "";
  std::string followingSubtitle = "";
  
  if ( PresentTitle != NULL ) { presentTitle = (std::string)PresentTitle; }
  if ( PresentSubtitle != NULL ) { presentSubtitle = (std::string)PresentSubtitle; }
  if ( FollowingTitle != NULL ) { followingTitle = (std::string)FollowingTitle; }
  if ( FollowingSubtitle != NULL ) { followingSubtitle = (std::string)FollowingSubtitle; }

  _osd = (BasicOsd*)new ProgrammeOsd(PresentTime, presentTitle, presentSubtitle, 
				     FollowingTime, (std::string)followingTitle, followingSubtitle);
}

StatusMonitor* StatusMonitor::get()
{
  static StatusMonitor sm;
  return &sm;
}

// --- DeleteOsdTask ---------------------------------------------------------

DeleteOsdTask::~DeleteOsdTask()
{
  if (osd != NULL)
  {
     if (osd->Type() == 0x01) {
        TextOsd* tosd = (TextOsd*)osd;
        tosd->ClearItems();
     }
     delete osd;
  }
}

