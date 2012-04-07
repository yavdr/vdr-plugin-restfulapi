#include "tools.h"
#include <list>
#include <vdr/status.h>


#ifndef __STATUSMONITOR_H
#define __STATUSMONITOR_H

class TextOsdItem
{
  private:
    std::string _text;
  public:
    TextOsdItem(std::string text) { _text = text; };
    ~TextOsdItem() { };
    std::string Text() { return _text; };
    void Text(std::string text) { _text = text; };
};

class TextOsd : BasicOsd
{
  private:
    std::string _title;
    std::string _message;
    
    std::string _red;
    std::string _green;
    std::string _yellow;
    std::string _blue;
    
    std::list< TextOsdItem* > _items;
    TextOsdItem*   _selected;
  public:
    TextOsd();
    ~TextOsd();
    virtual int Type() { return 0x01; }

    TextOsdItem*    GetItem(int i);
    TextOsdItem*    GetItem(std::string item);
    std::list< TextOsdItem* > GetItems() { return _items; };
    bool        ReplaceItem(TextOsdItem* item, int i);
    int         CountItems();
    void        ClearItems();
    void	AddItem(TextOsdItem* item);
    void        RemoveItem(TextOsdItem* item);
    void        RemoveItem(std::string item);

    std::string Title();
    std::string Message();
    std::string Red();
    std::string Green();
    std::string Yellow();
    std::string Blue();
    TextOsdItem* Selected();

    void        Title(std::string title); 
    void        Message(std::string message);
    void        Red(std::string red);
    void        Green(std::string green);
    void        Yellow(std::string yellow);
    void        Blue(std::string blue);
    void        Selected(TextOsdItem* item);
};

class ChannelOsd : BasicOsd
{
  private:
    std::string _channel;
  public:
    ChannelOsd(std::string channel) { _channel = channel; };
    ~ChannelOsd() { };
    virtual int Type() { return 0x02; }
    void Channel(std::string channel) { _channel = channel; };
    std::string Channel() { return _channel; };
};

class ProgrammeOsd : BasicOsd
{
  private:
    time_t _presentTime;
    std::string _presentTitle;
    std::string _presentSubtitle;
    time_t _followingTime;
    std::string _followingTitle;
    std::string _followingSubtitle;
  public:
    ProgrammeOsd(time_t PresentTime, std::string PresentTitle, std::string PresentSubtitle,
                 time_t FollowingTime, std::string FollowingTitle, std::string FollowingSubtitle);
    ~ProgrammeOsd() { };
    virtual int  Type() { return 0x03; };

    time_t       PresentTime() { return _presentTime; };
    void         PresentTime(time_t PresentTime) { _presentTime = PresentTime; };

    time_t       FollowingTime() { return _followingTime; };
    void         FollowingTime(time_t FollowingTime) { _followingTime = FollowingTime; };

    std::string  PresentTitle() { return _presentTitle; };
    void         PresentTitle(std::string PresentTitle) { _presentTitle = PresentTitle; };

    std::string  FollowingTitle() { return _followingTitle; };
    void         FollowingTitle(std::string FollowingTitle) { _followingTitle = FollowingTitle; };
    
    std::string  PresentSubtitle() { return _presentSubtitle; };
    void         PresentSubtitle(std::string PresentSubtitle) { _presentSubtitle = PresentSubtitle; };

    std::string  FollowingSubtitle() { return _followingSubtitle; };
    void         FollowingSubtitle(std::string FollowingSubtitle) { _followingSubtitle = FollowingSubtitle; };    
};

class StatusMonitor : public cStatus
{
  private:
    BasicOsd* _osd;
    int osd_type;
    int channel_number;
    cDevice *device;
    std::string recording_name;
    std::string recording_file;
    int volume;
    void OsdCreate(void);
    void OsdDestroy(void);
  protected:
    void TimerChange(const cTimer *Timer, eTimerChange Change);
#if APIVERSNUM >= 10726
    void ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView);
#else
    void ChannelSwitch(const cDevice *Device, int ChannelNumber);
#endif
    void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
    void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
    void SetVolume(int Volume, bool Absolute);
    void SetAudioTrack(int Index, const char * const *Tracks);
    void SetAudioChannel(int AudioChannel);
    void SetSubtitleTrack(int Index, const char * const *Tracks);
    void OsdClear(void);
    void OsdTitle(const char *Title);
    void OsdStatusMessage(const char *Message);
    void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
    void OsdItem(const char *Text, int Index);
    void OsdCurrentItem(const char *Text);
    void OsdTextItem(const char *Text, bool Scroll);
    void OsdChannel(const char *Text);
    void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);
  public:
    StatusMonitor() { _osd = NULL; };
    ~StatusMonitor() { };
    static StatusMonitor* get();
    BasicOsd* getOsd() { return _osd; }
    int getChannel() { return channel_number; }
    std::string getRecordingName() { return recording_name; }
    std::string getRecordingFile() { return recording_file; }
};

class DeleteOsdTask : BaseTask
{
  protected:
    BasicOsd* osd;
  public:
   DeleteOsdTask(BasicOsd* osd) { };
   ~DeleteOsdTask();
};

#endif

