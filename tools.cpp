#include "tools.h"
// --- Settings ----------------------------------------------------------------

std::string Settings::cutComment(std::string str)
{
  bool esc = false;
  char c;
  int counter = 0;
  while ( counter < (int)str.length() ) {
    c = str[counter];
    switch(c) {
      case '\\': if (!esc) esc = true; else esc = false;
                  break;
      case '#': if (!esc) return str.substr(0, counter);
                  break;
      default: esc = false;
                  break;
    }
    counter++;
  }
  return str;
}

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
  std::vector< std::string> parts = StringExtension::split(v, ".");
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

bool Settings::parseLine(std::string str)
{
  str = cutComment(str);
  int found = -1;
  std::string value, name;
  found = str.find_first_of('=');
  if ( found != -1 ) {
     value = StringExtension::trim(str.substr(found+1));
     name = str.substr(0, found);

     if ( (int)name.find("port") != -1 ) {
        if ( SetPort(value) ) {
           return true;
        } else {
           return false;
        }
     }

     if ( (int)name.find("ip") != -1 ) {
        if ( SetIp(value) ) {
           return true;
        } else {
           return false;
        }
     }

     if ( (int)name.find("epgimages") != -1 ) {
        if ( SetEpgImageDirectory(value) ) {
           return true;
        } else {
           return false;
        }
     }

     if ( (int)name.find("channellogos") != -1 ) {
        if ( SetChannelLogoDirectory(value) ) {
           return true;
        } else {
           return false;
        }
     }
  }
  return false;
}

Settings* Settings::get() 
{
  static Settings settings;
  return &settings;
}

void Settings::init()
{
  std::string configDir = cPlugin::ConfigDirectory() + (std::string)"/restfulapi.conf";
 
  FILE* f = fopen(configDir.c_str(), "r");
  if ( f != NULL ) {
     esyslog("restfulapi: trying to parse \"non debian specific\" restfulapi.conf");
     std::ostringstream data;
     char c;
     while ( !feof(f) ) {
       c = fgetc(f);
       if ( c == '\n' ) {
          parseLine(data.str());
          data.str(""); //.clear() is inherited from std::ios and does only clear the error state
       } else {
          if (!feof(f)) //don't add EOF-char to string
             data << c;
       }
     }
     parseLine(data.str());
     fclose(f);
  }
}

void Settings::initDefault()
{
  esyslog("restfulapi: loading default settings");

  SetPort((std::string)"8002");
  SetIp((std::string)"0.0.0.0");
  SetEpgImageDirectory((std::string)"/var/cache/vdr/epgimages");
  SetChannelLogoDirectory((std::string)"/usr/share/vdr/channel-logos");
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

void StreamExtension::write(std::string str)
{
  _out->write(str.c_str(), str.length());
}

void StreamExtension::writeHtmlHeader()
{
  write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
  write("<html xml:lang=\"en\" lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  write("<head>\n");
  write("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
  write("</head><body>");
}

void StreamExtension::writeXmlHeader()
{
  write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
}

bool StreamExtension::writeBinary(std::string path)
{
  std::ifstream* in = new std::ifstream(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
  bool result = false;
  if ( in->is_open() ) {

     int size = in->tellg();
     char* memory = new char[size];
     in->seekg(0, std::ios::beg);
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
  std::string dir;

  if ( _mode == FileNotifier::EVENTS) {
     dir = Settings::get()->EpgImageDirectory().c_str();
  } else {
     dir = Settings::get()->ChannelLogoDirectory().c_str();
  }
  
  _filedescriptor = inotify_init();
  _wd = -1;

  if ( dir.length() == 0 ) {
     esyslog("restfulapi: Initializing inotify for epgimages failed! (Check restfulapi-settings!)");
     _wd = -1;
  } else {
     _wd = inotify_add_watch( _filedescriptor, dir.c_str(), IN_CREATE | IN_DELETE );
     if ( _wd < 0 )
        esyslog("restfulapi: Initializing inotify for epgimages failed!");
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
                   esyslog("restfulapi: inotify: found new image: %s", event->name);
                   if ( _mode == FileNotifier::EVENTS )
                      FileCaches::get()->addEventImage((std::string)event->name);
                   else
                      FileCaches::get()->addChannelLogo((std::string)event->name);
                }
             
                if (event->mask & IN_DELETE)  {
                   esyslog("restfulapi: inotify: image %s has been removed", event->name);
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
  std::string imageFolder = Settings::get()->EpgImageDirectory();
  std::string folderWildcard = imageFolder + (std::string)"/*";
  VdrExtension::scanForFiles(folderWildcard, eventImages);
}

void FileCaches::cacheChannelLogos()
{
  std::string imageFolder = Settings::get()->ChannelLogoDirectory();
  std::string folderWildcard = imageFolder + (std::string)"/*";
  VdrExtension::scanForFiles(folderWildcard, channelLogos);
}

void FileCaches::searchEventImages(int eventid, std::vector< std::string >& files)
{
  cxxtools::Regex regex( StringExtension::itostr(eventid) + (std::string)"(_[0-9]+)?.[a-z]{3,4}" );
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
      std::string name = channelLogos[i];
      int delim = name.find_last_of(".");
      if ( delim != -1 ) { name = name.substr(0, delim); }

      if ( name == cname ) {
         return channelLogos[i];
      }
  }
  return "";
}

void FileCaches::addEventImage(std::string file)
{
  eventImages.push_back(file);
}

void FileCaches::addChannelLogo(std::string file)
{
  channelLogos.push_back(file);
}

void FileCaches::removeEventImage(std::string file)
{
  for (size_t i = 0; i < eventImages.size(); i++) {
      if ( eventImages[i] == file ) {
         eventImages[i] = "";
         break;
      }
  }
}

void FileCaches::removeChannelLogo(std::string file)
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

cChannel* VdrExtension::getChannel(std::string id)
{
  if ( id.length() == 0 ) return NULL;
 
  for (cChannel* channel = Channels.First(); channel; channel= Channels.Next(channel))
  {
      if ( id == (std::string)channel->GetChannelID().ToString() ) {
         return channel;
      }
  }
  return NULL;
}

cTimer* VdrExtension::getTimer(std::string id)
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

std::string VdrExtension::getTimerID(cTimer* timer)
{
  std::ostringstream str;
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
         std::string file(globbuf.gl_pathv[i]);
         size_t delimPos = file.find_last_of('/');
         files.push_back(file.substr(delimPos+1));
         found++;
     }
     globfree(&globbuf);
  }
  return found;
}

bool VdrExtension::doesFileExistInFolder(std::string wildcardpath, std::string filename)
{
  glob_t globbuf;
  globbuf.gl_offs = 0;
  if ( wildcardpath.empty() == false && glob(wildcardpath.c_str(), GLOB_DOOFFS, NULL, &globbuf) == 0) {
     for (size_t i = 0; i < globbuf.gl_pathc; i++) {
         std::string file(globbuf.gl_pathv[i]);
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

// --- StringExtension --------------------------------------------------------

std::string StringExtension::itostr(int i)
{
  std::stringstream str;
  str << i;
  return str.str();
}

int StringExtension::strtoi(std::string str)
{
  static cxxtools::Regex regex("[0-9]{1,}");
  if(!regex.match(str)) return -LOWINT; // lowest possible integer
  return atoi(str.c_str());
}

std::string StringExtension::replace(std::string const& text, std::string const& substring, std::string const& replacement)
{
  std::string result = text;
  std::string::size_type pos = 0;
  while ( ( pos = result.find( substring, pos ) ) != std::string::npos ) {
    result.replace( pos, substring.length(), replacement );
    pos += replacement.length();
  }
  return result;
}

std::string StringExtension::encodeToXml(const std::string &str)
{
    //source: http://www.mdawson.net/misc/xmlescape.php
    std::ostringstream result;

    for( std::string::const_iterator iter = str.begin(); iter!=str.end(); iter++ )
    {
         unsigned char c = (unsigned char)*iter;

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

    std::string res = result.str();
    std::string converted;
    utf8::replace_invalid(res.begin(), res.end(), back_inserter(converted));
    return converted;
}

cxxtools::String StringExtension::UTF8Decode(std::string str)
{
  static cxxtools::Utf8Codec utf8;
  std::string temp;
  utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp));
  return utf8.decode(temp);
}

std::string StringExtension::toLowerCase(std::string str)
{
  std::ostringstream res;
  for (int i=0;i<(int)str.length();i++)
  {
      res << (char)std::tolower(str[i]);
  }
  return res.str();
}

std::string StringExtension::trim(std::string str)
{
  int a = str.find_first_not_of(" \t");
  int b = str.find_last_not_of(" \t");
  if ( a == -1 ) a = 0;
  if ( b == -1 ) b = str.length() - 1;
  return str.substr(a, (b-a)+1);
}

std::vector< std::string > StringExtension::split(std::string str, std::string s)
{
  std::vector< std::string > result;
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

// --- QueryHandler -----------------------------------------------------------

QueryHandler::QueryHandler(std::string service, cxxtools::http::Request& request)
{
  _url = request.url();
  _service = service;
  _options.parse_url(request.qparams());
  //workaround for current cxxtools which always appends ascii character #012 at the end? AFAIK!
  _body.parse_url(request.bodyStr().substr(0,request.bodyStr().length()-1));

  std::string params = _url.substr(_service.length()/*, _url.length() - 1*/);
  parseRestParams(params);

  _format = "";
  if ( (int)_url.find(".xml") != -1 ) { _format = ".xml"; }
  if ( (int)_url.find(".json") != -1 ) { _format = ".json"; }
  if ( (int)_url.find(".html") != -1 ) { _format = ".html"; }
}

QueryHandler::~QueryHandler()
{

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
        std::string p = params.substr(start+1, (i-1)-(start));
        _params.push_back(p);
        start = i;
      }
    }
  }

  if(start != (int)params.length() - 1) {
    _params.push_back(params.substr(start + 1/*, params.length() - 1*/));
  }
}

std::string QueryHandler::getParamAsString(int level)
{
  if ( level >= (int)_params.size() )
     return "";

  std::string param = _params[level];
  if ( param == _format ) return "";
  if ( level == ((int)_params.size() -1) && _format != "" && param.length() > _format.length() ) {
     int f = param.find(_format);
     if ( f > 0 ) {
        return param.substr(0, f);
     } 
  }
  return param;
}

std::string QueryHandler::getOptionAsString(std::string name)
{
  return _options.param(name);
}

std::string QueryHandler::getBodyAsString(std::string name)
{
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

int QueryHandler::getBodyAsInt(std::string name)
{
  return StringExtension::strtoi(getOptionAsString(name));
}

bool QueryHandler::isFormat(std::string format)
{
  if ((int)_url.find(format) != -1)
     return true;
  return false;
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

RestfulService::RestfulService(std::string path, bool internal, int version, RestfulService* parent)
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

void RestfulServices::appendService(std::string path, bool internal, int version, RestfulService* parent)
{
  appendService(new RestfulService(path, internal, version, parent));
}

void RestfulServices::appendService(RestfulService* service)
{
  services.push_back(service);
}

std::vector< RestfulService* > RestfulServices::Services(bool internal, bool children)
{
  std::vector< RestfulService* > result;
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
  int now = time(NULL);
  BaseTask* bt = NULL;
  do
  {
    if ( bt != NULL) {
       tasks.pop_front();
       delete bt;
    }
    bt = tasks.front();
  }while(bt->Created()+1 > now);
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
