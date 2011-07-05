#include "tools.h"
#include <list>
#include <vdr/status.h>

#ifndef __STATUSMONITOR_H
#define __STATUSMONITOR_H

class OsdItem
{
  private:
    std::string _text;
  public:
    OsdItem(std::string text) { _text = text; };
    ~OsdItem() { };
    std::string Text() { return _text; };
    void Text(std::string text) { _text = text; };
};

class TextOsd
{
  private:
    std::string _title;
    std::string _message;
    std::string _text;
    
    std::string _red;
    std::string _green;
    std::string _yellow;
    std::string _blue;
    
    std::list< OsdItem* > _items;
    OsdItem*   _selected;
  public:
   TextOsd();
   ~TextOsd();

   OsdItem*    GetItem(int i);
   int         CountItems();
   void        ClearItems();
   void        RemoveItem(OsdItem* item);
   void        RemoveItem(std::string item);

   std::string Title();
   std::string Message();
   std::string Text();
   std::string Red();
   std::string Green();
   std::string Yellow();
   std::string Blue();
   OsdItem*    Selected();

   void        Title(const char* title); 
   void        Message(const char* message);
   void        Text(const char* text);
   void	       Red(const char* red);
   void        Green(const char* green);
   void        Yellow(const char* yellow);
   void        Blue(const char* blue);
   void        Selected(OsdItem* item);
};


class StatusMonitor : public cStatus
{
  private:
    TextOsd* _osd;
    int channel_number;
    cDevice *device;
    std::string recording_name;
    std::string recording_file;
    int volume;
    void OsdCreate(void);
  protected:
    virtual void TimerChange(const cTimer *Timer, eTimerChange Change);
    virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
    virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
    virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
    virtual void SetVolume(int Volume, bool Absolute);
    virtual void SetAudioTrack(int Index, const char * const *Tracks);
    virtual void SetAudioChannel(int AudioChannel);
    virtual void SetSubtitleTrack(int Index, const char * const *Tracks);
    virtual void OsdClear(void);
    virtual void OsdTitle(const char *Title);
    virtual void OsdStatusMessage(const char *Message);
    virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
    virtual void OsdItem(const char *Text, int Index);
    virtual void OsdCurrentItem(const char *Text);
    virtual void OsdTextItem(const char *Text, bool Scroll);
    virtual void OsdChannel(const char *Text);
    virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);
  public:
    StatusMonitor() { _osd = NULL; };
    ~StatusMonitor() { };
    static StatusMonitor* get();
    TextOsd* getOsd() { return _osd; }
};

#endif

