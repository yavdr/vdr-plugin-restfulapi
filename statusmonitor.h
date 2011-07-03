#include "tools.h"
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
    void Update(std::string text) { _text = text; };
};

class Osd
{
  private:
    std::string _title;
    std::string _message;
    std::string _text;
    
    std::string _red;
    std::string _green;
    std::string _yellow;
    std::string _blue;
    
    std::vector< OsdItem* > _items;
    OsdItem* _selected;
  public:
   Osd();
   ~Osd();
   void ClearItems();     
};


class StatusMonitor : public cStatus
{
  private:
    Osd* _osd;
  protected:
    virtual void OsdClear(void);
    virtual void OsdTitle(const char *Title);
    virtual void OsdStatusMessage(const char *Message);
    virtual void OsdItem(const char *Text, int Index);
    virtual void OsdCurrentItem(const char *Text);
    virtual void OsdTextItem(const char *Text, bool Scroll);
    virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
  public:
    StatusMonitor() { _osd = NULL; };
    ~StatusMonitor() { };
    static StatusMonitor* get();
    Osd* getOsd() { return _osd; }
};

#endif

