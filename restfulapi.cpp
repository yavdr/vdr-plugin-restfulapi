/*
 * restfulapi.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <getopt.h>
#include <vdr/plugin.h>
#include "serverthread.h"
#include "statusmonitor.h"

static const char *VERSION        = "0.2.6.5";
static const char *DESCRIPTION    = "Offers a RESTful-API to retrieve data from VDR";
static const char *MAINMENUENTRY  = NULL;//"Restfulapi";

class cPluginRestfulapi : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cServerThread serverThread;
  StatusMonitor* statusMonitor;
public:
  cPluginRestfulapi(void);
  virtual ~cPluginRestfulapi();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginRestfulapi::cPluginRestfulapi(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginRestfulapi::~cPluginRestfulapi()
{
  // Clean up after yourself!
}

const char *cPluginRestfulapi::CommandLineHelp(void)
{
  return "  -i 0.0.0.0,      --ip=0.0.0.0             ip of the interface on which the services should listen\n"
         "  -p 8002,         --port=8002              tcp port\n"
         "  -e ABC,          --epgimages=ABC          folder which stores the epg-images\n"
         "  -c DEF,          --channellogos=DEF       folder which stores the channel-logos\n"
	 "  -h false,        --headers=true           disable additional http headers for accessing the data by javascript even so it's on another port"
	 "  -w GHI,          --webapp=GHI             folder which stores a webapp";
}

bool cPluginRestfulapi::ProcessArgs(int argc, char *argv[])
{
  Settings* settings = Settings::get();
  esyslog("restfulapi: trying to parse command line arguments");

  static struct option long_options[] = {
       { "port",         required_argument,  NULL,  'p' },
       { "ip",           required_argument,  NULL,  'i' },
       { "epgimages",    required_argument,  NULL,  'e' },
       { "channellogos", required_argument,  NULL,  'c' },
       { "headers",      required_argument,  NULL,  'h' },
       { "webapp",       required_argument,  NULL,  'w' }
     };

  int optchar, optind = 0;

  while ( ( optchar = getopt_long( argc, argv, "p:i:e:c:h:w:", long_options, &optind ) ) != -1 ) {
     switch ( optchar ) {
        case 'p': settings->SetPort((std::string)optarg); break;
        case 'i': settings->SetIp((std::string)optarg); break;
        case 'e': settings->SetEpgImageDirectory((std::string)optarg);  break;
        case 'c': settings->SetChannelLogoDirectory((std::string)optarg); break;
        case 'h': settings->SetHeaders((std::string)optarg); break;
        case 'w': settings->SetWebappDirectory((std::string)optarg); break;
     };
  }

  std::string headers = "activated";
  if ( settings->Headers() == false ) { headers = "deactivated"; }

  esyslog("RESTful-API Settings: port: %i, ip: %s, eimgs: %s, cimgs: %s, webapp: %s, headers: %s",
          settings->Port(),
          settings->Ip().c_str(),
          settings->EpgImageDirectory().c_str(),
          settings->ChannelLogoDirectory().c_str(),
          settings->WebappDirectory().c_str(),
          headers.c_str());

  return true;
}

bool cPluginRestfulapi::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  statusMonitor = StatusMonitor::get();
  return true;
}

bool cPluginRestfulapi::Start(void)
{
  // Start any background activities the plugin shall perform.
  Settings* settings = Settings::get();

  std::string headers = "activated";
  if ( settings->Headers() == false ) { headers = "deactivated"; }

  esyslog("restfulapi: Used settings: port: %i, ip: %s, eimgs: %s, cimgs: %s, webapp: %s, headers: %s",
          settings->Port(),
          settings->Ip().c_str(),
          settings->EpgImageDirectory().c_str(),
          settings->ChannelLogoDirectory().c_str(),
          settings->WebappDirectory().c_str(),
          headers.c_str());

  settings->SetCacheDir((string) cPlugin::CacheDirectory(PLUGIN_NAME_I18N));
  settings->SetConfDir((string) cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));

  FileCaches::get(); //cache files

  string syncDir = Settings::get()->CacheDirectory() + "/sync";
  FileExtension::get()->exists(syncDir) || system(("mkdir -p " + syncDir).c_str());
  if (!FileExtension::get()->exists(syncDir)) {
      esyslog("restfulapi: error creating sync directory: %s", syncDir.c_str());
  } else {
      isyslog("restfulapi: using sync directory: %s", syncDir.c_str());
  }

  serverThread.Initialize();
  serverThread.Start();
  return true;
}

void cPluginRestfulapi::Stop(void)
{
  // Stop any background activities the plugin is performing. 
  FileCaches::get()->stopNotifier();
  serverThread.Stop();
  statusMonitor = NULL;
}

void cPluginRestfulapi::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.

  string cacheDir = Settings::get()->CacheDirectory();
  string syncDir = cacheDir + "/sync";
  string cmd = "find " + syncDir + " -type f -mtime +5 -delete";
  int result = system(cmd.c_str());
  if (result > 0) {
      esyslog("restfulapi: error cleaning up outdated syncfiles: %s", cmd.c_str());
  }
}

void cPluginRestfulapi::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
  TaskScheduler* scheduler = TaskScheduler::get();

  scheduler->DoTasks();
 
  tChannelID channelID = scheduler->SwitchableChannel();
  
#if APIVERSNUM > 20300
    LOCK_CHANNELS_READ;
    const cChannels& channels = *Channels;
#else
    cChannels& channels = Channels;
#endif

  if (!( channelID == tChannelID::InvalidID )) {
     const cChannel* channel = channels.GetByChannelID(channelID);
     if (channel != NULL) {
    	channels.SwitchTo( channel->Number() );
        scheduler->SwitchableChannel(tChannelID::InvalidID);
     }
  }

  const cRecording* recording = scheduler->SwitchableRecording();

  if (recording != NULL) {
     cReplayControl::SetRecording(recording->FileName());
     scheduler->SwitchableRecording(NULL);
     cControl::Shutdown();
     cControl::Launch(new cReplayControl);
  }
}

cString cPluginRestfulapi::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginRestfulapi::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginRestfulapi::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginRestfulapi::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginRestfulapi::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginRestfulapi::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginRestfulapi::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginRestfulapi::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginRestfulapi); // Don't touch this!
