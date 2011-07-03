#include "statusmonitor.h"

// --- Osd -------------------------------------------------------------------------------------------

Osd::Osd()
{
  _selected = NULL;
}

Osd::~Osd()
{
  ClearItems();
}

void Osd::ClearItems()
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

// --- StatusMonitor ---------------------------------------------------------------------------------

void StatusMonitor::OsdClear(void)
{
  if ( _osd != NULL ) 
     delete _osd;
  _osd = NULL;  
}

void StatusMonitor::OsdTitle(const char *Title)
{
  
}

void StatusMonitor::OsdStatusMessage(const char *Message)
{
  
}

void StatusMonitor::OsdItem(const char *Text, int Index)
{
  
}

void StatusMonitor::OsdCurrentItem(const char *Text)
{
  
}

void StatusMonitor::OsdTextItem(const char *Text, bool Scroll)
{
  
}

void StatusMonitor::OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue)
{

}

StatusMonitor* StatusMonitor::get()
{
  static StatusMonitor sm;
  return &sm;
}
