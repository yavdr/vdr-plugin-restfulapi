#include "tools.h"
#include <vdr/videodir.h>
using namespace std;

// --- Settings ----------------------------------------------------------------

bool Settings::SetPort(std::string v)
{
  int p = StringExtension::strtoi(v);
  if ( p > 1024 && p <= 65535 ) {
     port = p;
     esyslog("restfulapi: Port has been set to %i!", port);
     return true;
  }
  return false;
}

bool Settings::SetIp(std::string v)
{
  vector<string> parts = StringExtension::split(v, ".");
  if ( parts.size() != 4 ) {
     return false;
  }

  for (int i=0;i<(int)parts.size();i++) {
      int val = StringExtension::strtoi(parts[i]);
      if ( val < 0 || val > 255 ) {
         return false;
      }
  }
  ip = v;
  esyslog("restfulapi: Ip has been set to %s!", ip.c_str());
  return true;
}

bool Settings::SetEpgImageDirectory(std::string v)
{
  struct stat stat_info;
  if ( stat(v.c_str(), &stat_info) == 0) {
     if (v[v.length()-1] == '/')
        epgimage_dir = v.substr(0, v.length()-1);
     else
        epgimage_dir = v;
     esyslog("restfulapi: The EPG-Images will be loaded from %s!", epgimage_dir.c_str());
     return true;
  }
  return false;
}

bool Settings::SetChannelLogoDirectory(std::string v)
{
  struct stat stat_info;
  if ( stat(v.c_str(), &stat_info) == 0) {
     if (v[v.length()-1] == '/')
        channellogo_dir = v.substr(0, v.length()-1);
     else
        channellogo_dir = v;
     esyslog("restfulapi: The Channel-Logos will be loaded from %s!", channellogo_dir.c_str());
     return true;
  }
  return false;
}

bool Settings::SetHeaders(std::string v)
{
  if ( v == "false" ) {
     activateHeaders = false;
  } else {
     activateHeaders = true;
  }
  return true;
}

Settings* Settings::get() 
{
  static Settings settings;
  return &settings;
}

void Settings::initDefault()
{
  SetPort((string)"8002");
  SetIp((string)"0.0.0.0");
  SetEpgImageDirectory((string)"/var/cache/vdr/epgimages");
  SetChannelLogoDirectory((string)"/usr/share/vdr/channel-logos");
  SetHeaders((string)"true");
}

// --- HtmlHeader --------------------------------------------------------------

void HtmlHeader::ToStream(StreamExtension* se)
{
  se->write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  se->write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
  se->write("<html xml:lang=\"en\" lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  se->write("<head>\n");

  se->write("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
  // WebApp support
  se->write("<meta name=\"apple-mobile-web-app-capable\" content=\"yes\" />\n");
  se->write("<meta name=\"apple-mobile-web-app-status-bar-style\" content=\"default\" />\n");
  se->write("<meta name=\"viewport\" content=\"width=device-width\" id=\"viewport\" />\n");
  
  for (int i=0;i<(int)_metatags.size();i++)
  {
    se->write(_metatags[i].c_str());
    se->write("\n");
  }

  se->write("<title>");
  se->write(_title.c_str());
  se->write("</title>\n");

  se->write("<style type=\"text/css\">\n");
  for (int i=0;i<(int)_stylesheets.size();i++) 
  {
    se->writeBinary(_stylesheets[i]);
    se->write("\n");
  }
  se->write("</style>\n");
  
  se->write("<script type=\"text/javascript\">\n//<![CDATA[\n\n");

  for (int i=0;i<(int)_scripts.size();i++) 
  {
    se->writeBinary(_scripts[i]);
  }

  se->write("\n//]]>\n</script>\n");  

  if ( _onload.size() == 0 ) {
     se->write("</head><body>\n");
  } else {
     se->write("<body onload=\"");
     //f.e. javascript:bootstrap();
     se->write(_onload.c_str());
     se->write("\">\n");
  }
}

// --- StreamExtension ---------------------------------------------------------

StreamExtension::StreamExtension(std::ostream *out)
{
  _out = out;
}

std::ostream* StreamExtension::getBasicStream()
{
  return _out;
}

void StreamExtension::write(const char* str)
{
  std::string data = (std::string)str;
  _out->write(str, data.length());
}

void StreamExtension::writeHtmlHeader(std::string title)
{
  write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
  write("<html xml:lang=\"en\" lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  write("<head>\n");
  write("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
  // WebApp support
  write("<meta name=\"apple-mobile-web-app-capable\" content=\"yes\" />\n");
  write("<meta name=\"apple-mobile-web-app-status-bar-style\" content=\"default\" />\n");
  write("<meta name=\"viewport\" content=\"width=device-width\" id=\"viewport\" />\n");
    
  write("<title>VDR-Plugin-Restfulapi: ");
  if ( title.length() > 0 ) write(title.c_str());
  write("</title>");
  write("</head>\n<body>\n");
}

void StreamExtension::writeXmlHeader()
{
  write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
}

bool StreamExtension::writeBinary(std::string path)
{
  ifstream* in = new ifstream(path.c_str(), ios::in | ios::binary | ios::ate);
  bool result = false;
  if ( in->is_open() ) {

     int size = in->tellg();
     char* memory = new char[size];
     in->seekg(0, ios::beg);
     in->read(memory, size);
     _out->write(memory, size);
     delete[] memory;
     result = true;
  } 
  in->close();
  delete in;
  return result;
}

// --- FileNotifier -----------------------------------------------------------

FileNotifier::~FileNotifier()
{
  if ( active == true ) {
     Stop();
  }
}

void FileNotifier::Initialize(int mode)
{
  _mode = mode;
  string dir;

  if ( _mode == FileNotifier::EVENTS) {
     dir = Settings::get()->EpgImageDirectory().c_str();
  } else {
     dir = Settings::get()->ChannelLogoDirectory().c_str();
  }
  
  _filedescriptor = inotify_init();
  _wd = -1;

  if ( dir.length() == 0 ) {
     esyslog("restfulapi: Initializing inotify for epgimages or channellogos failed! (Check restfulapi-settings!)");
     _wd = -1;
  } else {
     _wd = inotify_add_watch( _filedescriptor, dir.c_str(), IN_CREATE | IN_DELETE );
     if ( _wd < 0 )
        esyslog("restfulapi: Initializing inotify for epgimages failed!");
     else
        esyslog("restfulapi: Initializing inotify for %s finished.", dir.c_str());
  }
 


  if (_wd >= 0) {
    active = true;
    Start();
  }
}

void FileNotifier::Action(void)
{
  int length, i;
  char buffer[BUF_LEN];

  while(active) {
    i = 0;
    struct pollfd pfd[1];
    pfd[0].fd = _filedescriptor;
    pfd[0].events = POLLIN;
    
    if ( poll(pfd, 1, 500) > 0 ) {
       length = read( _filedescriptor, buffer, BUF_LEN );
    
       if ( length > 0 ) {
          while ( i < length ) {
             struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
             if ( event->len > 0 && !(event->mask & IN_ISDIR) ) {
 
                if (event->mask & IN_CREATE) {
                   //esyslog("restfulapi: inotify: found new image: %s", event->name);
                   if ( _mode == FileNotifier::EVENTS )
                      FileCaches::get()->addEventImage((std::string)event->name);
                   else
                      FileCaches::get()->addChannelLogo((std::string)event->name);
                }
             
                if (event->mask & IN_DELETE)  {
                   //esyslog("restfulapi: inotify: image %s has been removed", event->name);
                   if ( _mode == FileNotifier::EVENTS )
                      FileCaches::get()->removeEventImage((std::string)event->name);
                   else
                      FileCaches::get()->removeChannelLogo((std::string)event->name);
                }
            
                i += EVENT_SIZE + event->len;
             }
          }
       }
    }
  }
  esyslog("restfulapi: Stopped watching directory!");
}

void FileNotifier::Stop()
{ 
  if ( active != false ) {
     active = false;
     Cancel(0.1);
     ( void ) inotify_rm_watch( _filedescriptor, _wd );
     ( void ) close( _filedescriptor );
  }
}

// --- FileCaches -------------------------------------------------------------

FileCaches* FileCaches::get()
{
  static FileCaches instance;
  return &instance;
}

void FileCaches::cacheEventImages()
{
  string imageFolder = Settings::get()->EpgImageDirectory();
  string folderWildcard = imageFolder + (string)"/*";
  VdrExtension::scanForFiles(folderWildcard, eventImages);
}

void FileCaches::cacheChannelLogos()
{
  string imageFolder = Settings::get()->ChannelLogoDirectory();
  string folderWildcard = imageFolder + (string)"/*";
  VdrExtension::scanForFiles(folderWildcard, channelLogos);
}

void FileCaches::searchEventImages(int eventid, std::vector< std::string >& files)
{
  cxxtools::Regex regex( (string)"^" + StringExtension::itostr(eventid) + (string)"(_[0-9]+)?.[a-z]{3,4}$" );
  for ( int i=0; i < (int)eventImages.size(); i++ ) {
      if ( regex.match(eventImages[i]) ) {
         files.push_back(eventImages[i]);
      }
  }
}

std::string FileCaches::searchChannelLogo(cChannel *channel)
{
  std::string cname = (std::string)channel->Name();
  
  for ( int i=0; i < (int)channelLogos.size(); i++ ) {
      string name = channelLogos[i];
      int delim = name.find_last_of(".");
      if ( delim != -1 ) { name = name.substr(0, delim); }

      if ( name == cname ) {
         return channelLogos[i];
      }
  }
  return "";
}

void FileCaches::addEventImage(string file)
{
  eventImages.push_back(file);
}

void FileCaches::addChannelLogo(string file)
{
  channelLogos.push_back(file);
}

void FileCaches::removeEventImage(string file)
{
  for (size_t i = 0; i < eventImages.size(); i++) {
      if ( eventImages[i] == file ) {
         eventImages[i] = "";
         break;
      }
  }
}

void FileCaches::removeChannelLogo(string file)
{
  for (size_t i = 0; i < channelLogos.size(); i++) {
      if ( channelLogos[i] == file ) {
         eventImages[i] = "";
         break;
      }
  }
}

// --- VdrExtension -----------------------------------------------------------

cChannel* VdrExtension::getChannel(int number)
{
  if( number == -1 || number >= Channels.Count() ) { return NULL; }

  cChannel* result = NULL;
  int counter = 1;
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
      if (!channel->GroupSep()) {
         if (counter == number)
         {
            result = channel;
            break;
         }
         counter++;
      }
  }
  return result;
}

cChannel* VdrExtension::getChannel(string id)
{
  if ( id.length() == 0 ) return NULL;
 
  for (cChannel* channel = Channels.First(); channel; channel= Channels.Next(channel))
  {
      if ( id == (string)channel->GetChannelID().ToString() ) {
         return channel;
      }
  }
  return NULL;
}

cTimer* VdrExtension::getTimer(string id)
{
  cTimer* timer;
  int tc = Timers.Count();
  for (int i=0;i<tc;i++) {
      timer = Timers.Get(i);
      if ( VdrExtension::getTimerID(timer) == id ) {
         return timer;
      }  
  }
  return NULL;
}

string VdrExtension::getTimerID(cTimer* timer)
{
  ostringstream str;
  str << (const char*)timer->Channel()->GetChannelID().ToString() << ":" << timer->WeekDays() << ":"
      << timer->Day() << ":"
      << timer->Start() << ":" << timer->Stop();
  return str.str();
}

int VdrExtension::scanForFiles(const std::string wildcardpath, std::vector< std::string >& files)
{
  int found = 0;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  if ( wildcardpath.empty() == false && glob(wildcardpath.c_str(), GLOB_DOOFFS, NULL, &globbuf) == 0) {
     for (size_t i = 0; i < globbuf.gl_pathc; i++) {
         string file(globbuf.gl_pathv[i]);
         size_t delimPos = file.find_last_of('/');
         files.push_back(file.substr(delimPos+1));
         found++;
     }
     globfree(&globbuf);
  }
  esyslog("restfulapi: found %i files in %s.", found, wildcardpath.c_str());
  return found;
}

bool VdrExtension::doesFileExistInFolder(string wildcardpath, string filename)
{
  glob_t globbuf;
  globbuf.gl_offs = 0;
  if ( wildcardpath.empty() == false && glob(wildcardpath.c_str(), GLOB_DOOFFS, NULL, &globbuf) == 0) {
     for (size_t i = 0; i < globbuf.gl_pathc; i++) {
         string file(globbuf.gl_pathv[i]);
         size_t delimPos = file.find_last_of('/');
         if (file.substr(delimPos+1) == filename) {
            globfree(&globbuf);
            return true;
         }
     }
     globfree(&globbuf);
  }
  return false;
}

bool VdrExtension::IsRadio(cChannel* channel)
{
  if ((channel->Vpid() == 0 && channel->Apid(0) != 0) || channel->Vpid() == 1 ) {
     return true;
  }
  return false;
}

bool VdrExtension::IsRecording(cRecording* recording)
{
  cTimer* timer = NULL;
  for (int i=0;i<Timers.Count();i++)
  {
     timer = Timers.Get(i);
     if (string(timer->File()).compare(recording->Name())) {
        return true;
     }
  }
  return false;
}

cTimer* VdrExtension::TimerExists(cEvent* event)
{
  for(int i=0;i<Timers.Count();i++) {
     cTimer* timer = Timers.Get(i);

     if ( timer->Event() != NULL &&  timer->Event()->EventID() == event->EventID() && strcmp(timer->Event()->ChannelID().ToString(), event->ChannelID().ToString()) == 0 ) {
        return timer;
     }
    
     if ( strcmp(timer->Channel()->GetChannelID().ToString(), event->ChannelID().ToString()) == 0 ) {
        int timer_start = (int)timer->Day() - ((int)timer->Day()) % 3600;
        int timer_stop = timer_start;
        timer_start += ((int)(timer->Start() / 100)) * 60 * 60;
        timer_start += (timer->Start() % 100) * 60;

        if ( timer->Stop() < timer->Start() ) timer_stop += 24 * 60 * 60;
        timer_stop += ((int)(timer->Stop() / 100)) * 60 * 60;
        timer_stop += (timer->Stop() % 100) * 60;

        if ( timer_stop >= (int)event->EndTime() && timer_start <= (int)event->StartTime() ) {
           return timer;
        }
     }
  }
  return NULL;
}

vector< cTimer* > VdrExtension::SortedTimers()
{
  vector< cTimer* > timers;
  for(int i=0;i<Timers.Count();i++)
  {
     timers.push_back(Timers.Get(i));
  }

  for(int i=0;i<(int)timers.size() - 1;i++)
  {
     bool changed = false;
     for(int k=0;k<(int)timers.size() - i - 1;k++)
     {
         if (VdrExtension::CompareTimers(timers[k], timers[k+1])) 
         {
            cTimer* swap = timers[k];
            timers[k] = timers[k+1];
            timers[k+1] = swap;
            changed = true;
         }
     }
     if(!changed) break;
  }

  return timers;
}

bool VdrExtension::CompareTimers(cTimer* timer1, cTimer* timer2)
{
  int day1 = (int)timer1->Day() - ((int)timer1->Day() % 3600);
  int day2 = (int)timer2->Day() - ((int)timer2->Day() % 3600);

  if (day1 > day2) {
     return true;
  } else if ( day1 < day2) {
     return false;
  } else {
     if (timer1->StartTime() > timer2->StartTime()) {
        return true;
     } else if (timer1->StartTime() < timer2->StartTime()) {
        return false;
     }
  }

  if ( ( (string)timer1->File() ).compare( (string)timer2->File() ) > 0 ) {
     return true;
  }

  return false;
}

int VdrExtension::RecordingLengthInSeconds(cRecording* recording)
{
  int nf = recording->NumFrames();
  if (nf >= 0)
#if APIVERSNUM >= 10703
     return int(((double)nf / recording->FramesPerSecond()));
#else
     return int((double)nf / FRAMESPERSEC);
#endif
  return -1;
}

cEvent* VdrExtension::GetEventById(tEventID eventID, cChannel* channel)
{
  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);

  if ( !Schedules ) return NULL;

  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
    const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
    if (Schedule) {
       cEvent* event = (cEvent*)Schedule->GetEvent(eventID);
       if ( event != NULL && (channel == NULL || strcmp(channel->GetChannelID().ToString(), event->ChannelID().ToString()) == 0 ) ) return event;
    }
  }
  return NULL;
}

string VdrExtension::getRelativeVideoPath(cRecording* recording)
{
  string path = (string)recording->FileName();
#if APIVERSNUM > 20101
  string VIDEODIR(cVideoDirectory::Name());
#else
  string VIDEODIR(VideoDirectory);
#endif
  return path.substr(VIDEODIR.length());
}

cEvent* VdrExtension::getCurrentEventOnChannel(cChannel* channel)
{
  if ( channel == NULL ) return NULL; 

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);

  if ( ! Schedules ) { return NULL; }
  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
  if ( !Schedule ) { return NULL; }

  time_t now = time(NULL);
  return (cEvent*)Schedule->GetEventAround(now);
}

// --- VdrMarks ---------------------------------------------------------------

VdrMarks* VdrMarks::get()
{
  static VdrMarks vdrMarks;
  return &vdrMarks;
}

string VdrMarks::cutComment(string str)
{
  bool esc = false;
  char c;
  int counter = 0;
  while ( counter < (int)str.length() ) {
    c = str[counter];
    switch(c) {
      case '\\': if (!esc) esc = true; else esc = false;
                  break;
      case ' ': if (!esc) return str.substr(0, counter);
                  break;
      default: esc = false;
                  break;
    }
    counter++;
  }
  return str;
}

bool VdrMarks::validateMark(string mark)
{
  static cxxtools::Regex regex("[0-9]{1,2}:[0-9]{2,2}:[0-9]{2,2}[.]{0,1}[0-9]{0,2}");
  return regex.match(mark);
}

string VdrMarks::getPath(cRecording* recording)
{
  string filename = recording->FileName();
  return filename + "/marks";
}

bool VdrMarks::parseLine(std::vector<string >& marks, string line)
{
  line = cutComment(line);
  if ( validateMark(line) ) {
     marks.push_back(line);
     return true;
  }
  return false;
}

vector<string > VdrMarks::readMarks(cRecording* recording)
{
  vector<string > marks;
  string path = getPath(recording);

  if ( path.length() == 0 ) {
     return marks;
  }

  FILE* f = fopen(path.c_str(), "r");
  if ( f != NULL ) {
     ostringstream data;
     char c;
     while ( !feof(f) ) {
       c = fgetc(f);
       if ( c == '\n' ) {
          parseLine(marks, data.str());
          data.str(""); //.clear() is inherited from std::ios and does only clear the error state
       } else {
          if (!feof(f)) //don't add EOF-char to string
             data << c;
       }
     }
     parseLine(marks, data.str());
     fclose(f);
  }

  return marks;
}

bool VdrMarks::saveMarks(cRecording* recording, std::vector< std::string > marks)
{
  if (recording == NULL) {
     return false;
  }

  for(int i=0;i<(int)marks.size();i++)
  {
     if ( !validateMark(marks[i]) ) {
        return false;
     }
  }

  string path = getPath(recording);
  if ( path.length() == 0 ) {
     return false;
  }

  deleteMarks(recording);

  FILE* file = fopen(path.c_str(), "w");
  if ( file != NULL ) {
     for(int i=0;i<(int)marks.size();i++) {
        for(int k=0;k<(int)marks[i].length();k++)
        {
           fputc( (int)marks[i][k], file );
        }
        fputc( (int)'\n', file );
     }

     fclose(file);
     return true;
  }

  return false;
}

bool VdrMarks::deleteMarks(cRecording* recording)
{
  string marksfile = getPath(recording);

  if ( remove( marksfile.c_str() ) != 0 ) {
     return false;
  }
  return true;
}

// --- StringExtension --------------------------------------------------------

string StringExtension::itostr(int i)
{
  stringstream str;
  str << i;
  return str.str();
}

int StringExtension::strtoi(string str)
{
  static cxxtools::Regex regex("[0-9]{1,}");
  if(!regex.match(str)) return -LOWINT; // lowest possible integer
  return atoi(str.c_str());
}

string StringExtension::replace(string const& text, string const& substring, string const& replacement)
{
  std::string result = text;
  std::string::size_type pos = 0;
  while ( ( pos = result.find( substring, pos ) ) != std::string::npos ) {
    result.replace( pos, substring.length(), replacement );
    pos += replacement.length();
  }
  return result;
}

string StringExtension::encodeToXml(const string &str)
{
    //source: http://www.mdawson.net/misc/xmlescape.php
    ostringstream result;

    for( string::const_iterator iter = str.begin(); iter!=str.end(); iter++ )
    {
         unsigned char c = (unsigned char)*iter;
         // filter invalid bytes in xml
         if (c < 0x20 && c != 0x09 && c != 0x0a && c != 0x0d)
            continue;

         switch( c )
         {
             case '&': result << "&amp;"; break;
             case '<': result << "&lt;"; break;
             case '>': result << "&gt;"; break;
             case '"': result << "&quot;"; break;
             case '\'': result << "&apos;"; break;

             default:
                   result << c;
         }
    }

    string res = result.str();
    try {
       string converted;
       utf8::replace_invalid(res.begin(), res.end(), back_inserter(converted));
       return converted;
    } catch(utf8::not_enough_room& e) {
       return (string)"Invalid piece of text. (Fixing Unicode failed.)";
    }
}


cxxtools::String StringExtension::encodeToJson(const string &str)
{
  // http://en.wikipedia.org/wiki/UTF-8#Codepage_layout
  ostringstream result;

  for( string::const_iterator iter = str.begin(); iter!=str.end(); iter++ )
  {
       unsigned char c = (unsigned char)*iter;
       if (c < 0x20)
          continue;
       else if (c == 0xc0 || c == 0xc1 || (c >= 0xf5 && c <= 0xff))
          result << (0xc0 | (c >> 6)) << (0x80 | (c & 0x3f));
       else
          result << c;
  }

  string res = result.str();
  static cxxtools::Utf8Codec utf8;
  try {
     string converted;
     utf8::replace_invalid(res.begin(), res.end(), back_inserter(converted));
     return utf8.decode(converted);
  } catch(utf8::not_enough_room& e) {
     return utf8.decode((string)"Invalid piece of text. (Fixing Unicode failed.)");
  }
}


cxxtools::String StringExtension::UTF8Decode(string str)
{
  static cxxtools::Utf8Codec utf8;
  try {
     string converted;
     utf8::replace_invalid(str.begin(), str.end(), back_inserter(converted));
     return utf8.decode(converted);
  } catch (utf8::not_enough_room& e) {
     return utf8.decode((string)"Invalid piece of text. (Fixing Unicode failed.)");
  }
}

string StringExtension::toLowerCase(string str)
{
  ostringstream res;
  for (int i=0;i<(int)str.length();i++)
  {
      res << (char)tolower(str[i]);
  }
  return res.str();
}

string StringExtension::trim(string str)
{
  int a = str.find_first_not_of(" \t");
  int b = str.find_last_not_of(" \t");
  if ( a == -1 ) a = 0;
  if ( b == -1 ) b = str.length() - 1;
  return str.substr(a, (b-a)+1);
}

vector<string > StringExtension::split(string str, string s)
{
  vector< string > result;
  if ( str.length() <= 1 ) return result;

  int found = 0;
  int previous = -1;
  while((found = str.find_first_of(s.c_str(), found+1)) != -1 ) {
    result.push_back(str.substr(previous+1, (found+(-previous-1))));
    previous = found;
  }
  found++;
  result.push_back(str.substr(previous+1));

  return result;
}

string StringExtension::timeToString(time_t time)
{
  struct tm *ltime = localtime(&time);
  char buffer[26];
  strftime(buffer, 26, "%H:%M", ltime);
  return (std::string)buffer;
}

string StringExtension::dateToString(time_t time)
{
  struct tm *ltime = localtime(&time);

  ostringstream data;
  data << StringExtension::addZeros((ltime->tm_year+1900), 4) << "-"
       << StringExtension::addZeros((ltime->tm_mon+1), 2) << "-"
       << StringExtension::addZeros((ltime->tm_mday), 2) << " "
       << StringExtension::addZeros((ltime->tm_hour), 2) << ":"
       << StringExtension::addZeros((ltime->tm_min), 2) << ":"
       << StringExtension::addZeros((ltime->tm_sec), 2);
  return data.str();
}

string StringExtension::addZeros(int value, int digits)
{
  string strValue = StringExtension::itostr(value);
  if ( value < 0 ) return strValue;

  while ( (int)strValue.length() < digits ) {
    strValue = "0" + strValue;
  }

  return strValue;
}

// --- QueryHandler -----------------------------------------------------------

QueryHandler::QueryHandler(string service, cxxtools::http::Request& request)
{
  _url = request.url();
  _service = service;
  _options.parse_url(request.qparams());
  //workaround for current cxxtools which always appends ascii character #012 at the end? AFAIK!
  string body = request.bodyStr().substr(0,request.bodyStr().length()-1);
  bool found_json = false;
 
  int i = 0;
  while(!found_json && body.length() > (size_t)i) {
    if (body[i] == '{') {
       found_json = true;
    } else if (body[i] != '\t' && body[i] != '\n' && body[i] != ' ') {
       break;
    }
    i++;
  }  

  if ( found_json ) {
     jsonObject = jsonParser.Parse(body);
     esyslog("restfulapi: JSON parsed successfully: %s", jsonObject == NULL ? "no" : "yes");
  } else {
     _body.parse_url(body);
     jsonObject = NULL;
  }

  string params = _url.substr(_service.length());
  parseRestParams(params);

  _format = "";
  if ( (int)_url.find(".xml") != -1 ) { _format = ".xml"; }
  if ( (int)_url.find(".json") != -1 ) { _format = ".json"; }
  if ( (int)_url.find(".html") != -1 ) { _format = ".html"; }
}

QueryHandler::~QueryHandler()
{
  if (jsonObject != NULL) {
     delete jsonObject;
  }
}

void QueryHandler::parseRestParams(std::string params)
{
  int start = -1;

  for(int i=0;i<(int)params.length();i++)
  {
    if(params[i] == '/')
    {
      if(start == -1)
      {
        start = i;
      } else {
        string p = params.substr(start+1, (i-1)-(start));
        _params.push_back(p);
        start = i;
      }
    }
  }

  if(start != (int)params.length() - 1) {
    _params.push_back(params.substr(start + 1));
  }
}

string QueryHandler::getJsonString(string name)
{
  if ( jsonObject == NULL ) return "";
  JsonValue* jsonValue = jsonObject->GetItem(name);
  if ( jsonValue == NULL ) return "";
  JsonBase* jsonBase = jsonValue->Value();
  if ( jsonBase == NULL || !jsonBase->IsBasicValue()) return "";
  JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
  if ( jsonBasicValue->IsString() ) return jsonBasicValue->ValueAsString();
  if ( jsonBasicValue->IsBool() ) return jsonBasicValue->ValueAsBool() ? "true" : "false";
  std::ostringstream str;
  if ( jsonBasicValue->IsDouble() ) { str << jsonBasicValue->ValueAsDouble(); return str.str(); }
  return "";
}

int QueryHandler::getJsonInt(std::string name)
{
  if ( jsonObject == NULL ) return -LOWINT;
  JsonValue* jsonValue = jsonObject->GetItem(name);
  if ( jsonValue == NULL ) return -LOWINT;
  JsonBase* jsonBase = jsonValue->Value();
  if ( jsonBase == NULL || !jsonBase->IsBasicValue()) return -LOWINT;
  JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
  if ( jsonBasicValue->IsBool() ) return jsonBasicValue->ValueAsBool() ? 1 : 0;
  if ( jsonBasicValue->IsDouble() ) return (int)jsonBasicValue->ValueAsDouble();
  return -LOWINT;
}

bool QueryHandler::getJsonBool(std::string name)
{
  if (jsonObject == NULL) return false;
  JsonValue* jsonValue = jsonObject->GetItem(name);
  if (jsonValue == NULL) return false;
  JsonBase* jsonBase = jsonValue->Value();
  if (jsonBase == NULL || !jsonBase->IsBasicValue()) return false;
  JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
  if (jsonBasicValue->IsBool()) return jsonBasicValue->ValueAsBool();
  if (jsonBasicValue->IsDouble()) return jsonBasicValue->ValueAsDouble() != 0 ? true : false;
  return false;
}

string QueryHandler::getParamAsString(int level)
{
  if ( level >= (int)_params.size() )
     return "";

  string param = _params[level];
  if ( param == _format ) return "";
  if ( level == ((int)_params.size() -1) && _format != "" && param.length() > _format.length() ) {
     int f = param.find(_format);
     if ( f > 0 ) {
        return param.substr(0, f);
     } 
  }
  return param;
}

string QueryHandler::getOptionAsString(string name)
{
  return _options.param(name);
}

string QueryHandler::getBodyAsString(string name)
{
  if (jsonObject != NULL) {
     return getJsonString(name);
  }
  return _body.param(name);
}

int QueryHandler::getParamAsInt(int level)
{
  return StringExtension::strtoi(getParamAsString(level));  
}

int QueryHandler::getOptionAsInt(std::string name)
{
  return StringExtension::strtoi(getOptionAsString(name));
}

int QueryHandler::getBodyAsInt(string name)
{
  if (jsonObject != NULL) {
     return getJsonInt(name);
  }
  return StringExtension::strtoi(getBodyAsString(name));
}

bool QueryHandler::getBodyAsBool(string name)
{
  if (jsonObject != NULL) {
     return getJsonBool(name);
  }
  string result = getBodyAsString(name);
  if (result == "true") return true;
  if (result == "false") return false;
  if (result == "1") return true;
  return false;
}

JsonArray* QueryHandler::getBodyAsArray(string name)
{
  if ( jsonObject == NULL ) {
     return NULL;
  }
  JsonValue* jsonValue = jsonObject->GetItem(name);
  if ( jsonValue == NULL || jsonValue->Value() == NULL || !jsonValue->Value()->IsArray() ) {
     return NULL;
  }
  return (JsonArray*)jsonValue->Value();
}

bool QueryHandler::isFormat(string format)
{
  if ((int)_url.find(format) != -1)
     return true;
  return false;
}

void QueryHandler::addHeader(cxxtools::http::Reply& reply)
{
  reply.addHeader("Access-Control-Allow-Origin", "*");
  reply.addHeader("Access-Control-Allow-Methods", "POST, GET, DELETE, PUT");
}

// --- BaseList ---------------------------------------------------------------

BaseList::BaseList()
{
  iterator = 0;
  counter = 0;
  start = -1;
  limit = -1;
}

void BaseList::activateLimit(int _start, int _limit)
{
  if ( _start >= 0 && _limit >= 1 ) {
     start = _start;
     limit = _limit;
  }
}

bool BaseList::filtered()
{
  if ( start != -1 && limit != -1 ) {
     if (iterator >= start && iterator < (start+limit)) {
        counter++;
        iterator++;
        return false;
     }
     iterator++;
     return true;
  } else { 
     counter++;
     return false;
  }
}

// --- RestfulService ---------------------------------------------------------

RestfulService::RestfulService(string path, bool internal, int version, RestfulService* parent)
{
  _path = path;
  _internal = internal;
  _regex = new cxxtools::Regex(path + (std::string)"*");
  _version = version;
  _parent = parent;
}

RestfulService::~RestfulService()
{
  delete _regex;
}

// --- RestfulServices --------------------------------------------------------

RestfulServices::~RestfulServices()
{
  while( (int)services.size() != 0 ) {
    RestfulService* s = services.back();
    services.pop_back();
    delete s;
  }
}

RestfulServices* RestfulServices::get()
{
  static RestfulServices rs;
  return &rs;
}

void RestfulServices::appendService(string path, bool internal, int version, RestfulService* parent)
{
  appendService(new RestfulService(path, internal, version, parent));
}

void RestfulServices::appendService(RestfulService* service)
{
  services.push_back(service);
}

vector< RestfulService* > RestfulServices::Services(bool internal, bool children)
{
  vector< RestfulService* > result;
  for (size_t i = 0; i < services.size(); i++) {
    if (((services[i]->Internal() && internal) || !internal) || (children || (!children && services[i]->Parent() == NULL))) {
       result.push_back(services[i]);
    }
  }
  return result;
}

// --- TaskScheduler ---------------------------------------------------------

TaskScheduler* TaskScheduler::get()
{
  static TaskScheduler ts;
  return &ts;
}

void TaskScheduler::DoTasks()
{
  /*int now = time(NULL);
  BaseTask* bt = NULL;
  if ( tasks.size() > 0 ) {
     do
     {
       if ( bt != NULL) {
          if ( bt->Created()+1 > now) {
             tasks.pop_front();
             delete bt;
             break;
          }
       }
       bt = tasks.front();
     }while(bt != NULL);
  }*/
}

TaskScheduler::~TaskScheduler()
{
  BaseTask* bt = NULL;
  do
  {
    if (bt != NULL) 
    {
       tasks.pop_front();
       delete bt;
    }
    bt = tasks.front();
  }while(bt != NULL);
}

void TaskScheduler::SwitchableChannel(tChannelID channel)
{
  _channelMutex.Lock();
  _channel = channel;
  _channelMutex.Unlock();
}

tChannelID TaskScheduler::SwitchableChannel()
{
  _channelMutex.Lock();
  tChannelID tmp = _channel;
  _channel = tChannelID::InvalidID;
  _channelMutex.Unlock();
  return tmp;
}
