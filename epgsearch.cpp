#include "epgsearch.h"

using namespace std;

void operator<<= (cxxtools::SerializationInfo& si, SerSearchTimerContainer s)
{
  si.addMember("id") <<= s.timer->Id();
  si.addMember("search") <<= StringExtension::UTF8Decode(s.timer->Search());
  si.addMember("mode") <<= s.timer->SearchMode();
  si.addMember("tolerance") <<= s.timer->Tolerance();
  si.addMember("match_case") <<= s.timer->MatchCase();

  si.addMember("use_title") <<= s.timer->UseTitle();
  si.addMember("use_subtitle") <<= s.timer->UseSubtitle();
  si.addMember("use_description") <<= s.timer->UseDescription();
  si.addMember("content_descriptors") <<= s.timer->ContentRecognition();

  si.addMember("use_ext_epg_info") <<= s.timer->UseExtEPGInfo();
  si.addMember("ext_epg_info") <<= s.timer->ExtEPGInfo();

  si.addMember("use_in_favorites") <<= s.timer->UseInFavorites();

  si.addMember("use_time") <<= s.timer->UseTime();
  si.addMember("start_time") <<= s.timer->StartTime();
  si.addMember("stop_time") <<= s.timer->StopTime();

  si.addMember("use_channel") <<= s.timer->UseChannel();
  si.addMember("channel_min") <<= StringExtension::UTF8Decode((const char*)s.timer->ChannelMin().ToString());
  si.addMember("channel_max") <<= StringExtension::UTF8Decode((const char*)s.timer->ChannelMax().ToString());
  si.addMember("channels") <<= StringExtension::UTF8Decode(s.timer->ChannelText());

  si.addMember("use_duration") <<= s.timer->UseDuration();
  si.addMember("duration_min") <<= s.timer->MinDuration();
  si.addMember("duration_max") <<= s.timer->MaxDuration();

  si.addMember("use_dayofweek") <<= s.timer->UseDayOfWeek();
  si.addMember("dayofweek") <<= s.timer->DayOfWeek();

  si.addMember("use_as_searchtimer") <<= s.timer->UseAsSearchTimer();
  si.addMember("use_as_searchtimer_from") <<= s.timer->UseAsSearchTimerFrom();
  si.addMember("use_as_searchtimer_til") <<= s.timer->UseAsSearchTimerTil();
  si.addMember("search_timer_action") <<= s.timer->SearchTimerAction();
  si.addMember("use_series_recording") <<= s.timer->UseSeriesRecording();
  si.addMember("directory") <<= s.timer->Directory();
  si.addMember("del_recs_after_days") <<= s.timer->DelRecsAfterDays();
  si.addMember("keep_recs") <<= s.timer->KeepRecs();
  si.addMember("pause_on_recs") <<= s.timer->PauseOnRecs();
  si.addMember("blacklist_mode") <<= s.timer->BlacklistMode();
  si.addMember("blacklist_ids") <<= s.timer->BlacklistIds();
  si.addMember("switch_min_before") <<= s.timer->SwitchMinBefore();
  si.addMember("avoid_repeats") <<= s.timer->AvoidRepeats();
  si.addMember("allowed_repeats") <<= s.timer->AllowedRepeats();
  si.addMember("repeats_within_days") <<= s.timer->RepeatsWithinDays();
  si.addMember("compare_title") <<= s.timer->CompareTitle();
  si.addMember("compare_subtitle") <<= s.timer->CompareSubtitle();
  si.addMember("compare_summary") <<= s.timer->CompareSummary();
  si.addMember("compare_categories") <<= s.timer->CompareCategories();
  si.addMember("priority") <<= s.timer->Priority();
  si.addMember("lifetime") <<= s.timer->Lifetime();
  si.addMember("margin_start") <<= s.timer->MarginStart();
  si.addMember("margin_stop") <<= s.timer->MarginStop();
  si.addMember("use_vps") <<= s.timer->UseVPS();
  si.addMember("del_mode") <<= s.timer->DelMode();
  si.addMember("del_after_count_recs") <<= s.timer->DelAfterCountRecs();
  si.addMember("del_after_days_of_first_rec") <<= s.timer->DelAfterDaysOfFirstRec();

  si.addMember("ignore_missing_epg_cats") <<= s.timer->IgnoreMissingEPGCats();
  si.addMember("unmute_sound_on_switch") <<= s.timer->UnmuteSoundOnSwitch();
  si.addMember("summary_match") <<= s.timer->SummaryMatch();
  si.addMember("compare_time") <<= s.timer->CompareTime();
}

namespace vdrlive {


string SearchTimer::ToXml()
{
  ostringstream s;
  s << "<searchtimer>\n"
  << "<id>" << Id() << "</id>\n"
  << "<search>" << StringExtension::encodeToXml(Search()) << "</search>\n"
  << "<mode>" << SearchMode() << "</mode>\n"
  << "<tolerance>" << Tolerance() << "</tolerance>\n"
  << "<match_case>" << MatchCase() << "</match_case>\n"
  << "<use_time>" << UseTime() << "</use_time>\n"
  << "<use_title>" << UseTitle() << "</use_title>\n"
  << "<use_subtitle>" << UseSubtitle() << "</use_subtitle>\n"
  << "<start_time>" << StartTime() << "</start_time>\n"
  << "<stop_time>" << StopTime() << "</stop_time>\n"
  << "<use_channel>" << UseChannel() << "</use_channel>\n"
  << "<channel_min>" << ChannelMin() << "</channel_min>\n"
  << "<channel_max>" << ChannelMax() << "</channel_max>\n"
  << "<channels>" << StringExtension::UTF8Decode(ChannelText()) << "</channels>\n"
  << "<use_as_searchtimer>" << UseAsSearchTimer() << "</use_as_searchtimer>\n"
  << "<use_as_searchtimer_from>" << UseAsSearchTimerTil() << "</use_as_searchtimer_from>\n"
  << "<use_as_searchtimer_til>" << UseAsSearchTimerFrom() << "</use_as_searchtimer_til>\n"
  << "<use_duration>" << UseDuration() << "</use_duration>\n"
  << "<duration_min>" << MinDuration() << "</duration_min>\n"
  << "<duration_max>" << MaxDuration() << "</duration_max>\n"
  << "<use_dayofweek>" << UseDayOfWeek() << "</use_dayofweek>\n"
  << "<dayofweek>" << DayOfWeek() << "</dayofweek>\n"
  << "<use_in_favorites>" << UseInFavorites() << "</use_in_favorites>\n"
  << "<directory>" << Directory() << "</directory>\n"
  << "<del_recs_after_days>" << DelRecsAfterDays() << "</del_recs_after_days>\n"
  << "<keep_recs>" << KeepRecs() << "</keep_recs>\n"
  << "<pause_on_recs>" << PauseOnRecs() << "</pause_on_recs>\n"
  << "<blacklist_mode>" << BlacklistMode() << "</blacklist_mode>\n"
  << "<blacklist_ids>\n";
  vector< int > blacklists = BlacklistIds();
  for (int i=0;i<(int)blacklists.size();i++)
  {
    s << " <id>" << blacklists[i] << "</id>\n";
  }
  s << "</blacklist_ids>\n"
  << "<switch_min_before>" << SwitchMinBefore() << "</switch_min_before>\n"
  << "<use_ext_epg_info>" << UseExtEPGInfo() << "</use_ext_epg_info>\n"
  << "<ext_epg_info>\n";
  vector<string > epg_infos = ExtEPGInfo();
  for (int i=0;i<(int)epg_infos.size();i++)
  {
    s << " <info>" << epg_infos[i] << "</info>\n";
  }
  s << "</ext_epg_info>\n"
  << "<avoid_repeats>" << AvoidRepeats() << "</avoid_repeats>\n"
  << "<allowed_repeats>" << AllowedRepeats() << "</allowed_repeats>\n"
  << "<repeats_within_days>" << RepeatsWithinDays() << "</repeats_within_days>\n"
  << "<compare_title>" << CompareTitle() << "</compare_title>\n"
  << "<compare_subtitle>" << CompareSubtitle() << "</compare_subtitle>\n"
  << "<compare_summary>" << CompareSummary() << "</compare_summary>\n"
  << "<compare_categories>" << CompareCategories() << "</compare_categories>\n"
  << "<priority>" << Priority() << "</priority>\n"
  << "<lifetime>" << Lifetime() << "</lifetime>\n"
  << "<margin_start>" << MarginStart() << "</margin_start>\n"
  << "<margin_stop>" << MarginStop() << "</margin_stop>\n"
  << "<use_vps>" << UseVPS() << "</use_vps>\n"
  << "<del_mode>" << DelMode() << "</del_mode>\n"
  << "<del_after_count_recs>" << DelAfterCountRecs() << "</del_after_count_recs>\n"
  << "<del_after_days_of_first_rec>" << DelAfterDaysOfFirstRec() << "</del_after_days_of_first_rec>\n"
  << "<ignore_missing_epg_cats>" << IgnoreMissingEPGCats() << "</ignore_missing_epg_cats>\n"
  << "<unmute_sound_on_switch>" << UnmuteSoundOnSwitch() << "</unmute_sound_on_switch>\n"
  << "<summary_match>" << SummaryMatch() << "</summary_match>\n"
  << "<content_descriptors>" << ContentRecognition() << "</content_descriptors>\n"
  << "<compare_time>" << CompareTime() << "</compare_time>\n"
  << "</searchtimer>\n";
  
  return s.str();
}

string SearchTimer::ToHtml()
{
  return Search();
}

string SearchTimer::LoadCommonFromQuery(QueryHandler& q) {
  int searchtimer_id = q.getBodyAsInt("id");
  if ( searchtimer_id >= 0 ) m_id = searchtimer_id;

  string search = q.getBodyAsString("search");
  if ( search.length() > 0 ) { m_search = search; } else { return "Search required."; }

  m_mode = q.getBodyAsInt("mode");
  if ( m_mode < 0 | m_mode > 5 )  { //0=phrase, 1=all words, 2=at least one word, 3=match exactly, 4=regex, 5=fuzzy
      m_mode = 0;
  }

  //only required in fuzzy mode
  int tolerance = q.getBodyAsInt("tolerance");
  if ( tolerance >= 0 && m_mode == 5 ) m_fuzzytolerance = tolerance;

  m_useCase = q.getBodyAsBool("match_case");

  m_useTitle = q.getBodyAsBool("use_title");
  m_useSubtitle = q.getBodyAsBool("use_subtitle");
  m_useDescription = q.getBodyAsBool("use_description");
  if ( !m_useTitle && !m_useSubtitle && !m_useDescription ) {
      return "You have to select at least one of the following three values: use_title, use_subtitle or/and use_description.";
  }

  m_contentDescriptors = q.getBodyAsString("content_descriptors");

  // ext EPG Info
  m_useExtEPGInfo = q.getBodyAsBool("use_ext_epg_info");

  JsonArray *extepg = q.getBodyAsArray("ext_epg_info");
  if (extepg != NULL) {
    for (int i=0; i < extepg->CountItem(); i++) {
	JsonBase* jsonBase = extepg->GetItem(i);
	if (jsonBase->IsBasicValue()) {
	    JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
	    if (jsonBasicValue->IsString()) {
		m_ExtEPGInfo.push_back(jsonBasicValue->ValueAsString()) ;
	    }
	}
    }
  }
  m_ignoreMissingEPGCats = q.getBodyAsBool("ignore_missing_epg_cats");


  m_useTime = q.getBodyAsBool("use_time");
  if ( m_useTime ) {
     m_startTime = q.getBodyAsInt("start_time");
     m_stopTime = q.getBodyAsInt("stop_time");
     if ( m_startTime < 0 || m_stopTime < 0 ) return "start or stop-time invalid";
  }

  int use_channel = q.getBodyAsInt("use_channel");
  if ( use_channel >= 0 && use_channel <= 3 ) m_useChannel = use_channel; else return "use_channel invalid (0=no, 1=interval, 2=channel group, 3=only FTA";
  if ( m_useChannel == Interval ) {
	 const cChannel* minChannel = VdrExtension::getChannel(q.getBodyAsString("channel_min"));
	 const cChannel* maxChannel = VdrExtension::getChannel(q.getBodyAsString("channel_max"));
     if (minChannel == NULL || maxChannel == NULL) {
        return "channel_min or channel_max invalid";
     }
     
     m_channelMin = minChannel->GetChannelID();
     m_channelMax = maxChannel->GetChannelID();
  } else if(use_channel != 0) {
     m_channels = q.getBodyAsString("channels");
     if ( m_channels.length() == 0 ) { return "use_channels activated but no channel selected"; }
  }
  
  m_useDuration = q.getBodyAsBool("use_duration");
  if ( m_useDuration ) {
     m_minDuration = q.getBodyAsInt("duration_min");
     m_maxDuration = q.getBodyAsInt("duration_max");
     if ( m_minDuration < 0 || m_maxDuration < 1 ) return "min/max-duration invalid";
  }
  
  m_useDayOfWeek = q.getBodyAsBool("use_dayofweek");
  if ( m_useDayOfWeek ) {
     m_dayOfWeek = q.getBodyAsInt("dayofweek");
     if (m_dayOfWeek < -127 || m_dayOfWeek > 6 ) return "day_of_week invalid (uses 7 bits for the seven days!)";
  }

  //m_blacklistmode: 0=global, 1=Selection, 2=all, 3=none
  m_blacklistmode = q.getBodyAsInt("blacklist_mode");

  if (m_blacklistmode == 1) {
    JsonArray *blacklistIds = q.getBodyAsArray("blacklist_ids");
    if (blacklistIds != NULL) {
      for (int i=0; i < blacklistIds->CountItem(); i++) {
	JsonBase* jsonBase = blacklistIds->GetItem(i);
	if (jsonBase->IsBasicValue()) {
	  JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
	  if (jsonBasicValue->IsString()) {
	    int id = atoi(jsonBasicValue->ValueAsString().c_str());
	    m_blacklistIDs.push_back(id);
	  }
	}
      }
    }
  }

  return "";
};

string SearchTimer::LoadFromQuery(QueryHandler& q)
{

  string result = LoadCommonFromQuery(q);
  if (result.length() > 0) {
      return result;
  }

  m_useEpisode = q.getBodyAsBool("use_series_recording");

  int priority = q.getBodyAsInt("priority");
  if (priority >= 0) m_priority = priority;

  int lifetime = q.getBodyAsInt("lifetime");
  if (lifetime >= 0) m_lifetime = lifetime;

  m_useInFavorites = q.getBodyAsBool("use_in_favorites");

  int useAsSearchtimer = q.getBodyAsInt("use_as_searchtimer");
  if (useAsSearchtimer >= 0 && useAsSearchtimer <= 2) 
     m_useAsSearchtimer = useAsSearchtimer;
  else
     return "use_as_searchtimer invalid (0=no, 1=yes, 2=user defined";

  if (useAsSearchtimer == 2) {

     int f = q.getBodyAsInt("use_as_searchtimer_from");
     int t = q.getBodyAsInt("use_as_searchtimer_til");
     if (f > 0) {
	 m_useAsSearchTimerFrom = f;
	 m_useAsSearchTimerTil = t > 0 ? t : 0;
     } else {
	 return "user defined start time not valid (use unix timestamp)";
     }
  }

  int action = q.getBodyAsInt("search_timer_action");
  if ( action >= 0 ) m_action = action;

  string directory = q.getBodyAsString("directory");
  if ( directory.length() > 0 ) m_directory = directory;

  int del_after_days = q.getBodyAsInt("del_recs_after_days");
  if ( del_after_days > 0 ) m_delAfterDays = del_after_days;
 
  int keep_recs = q.getBodyAsInt("keep_recs");
  if ( keep_recs > 0 ) m_recordingsKeep = keep_recs;

  int pause_on_recs = q.getBodyAsInt("pause_on_recs");
  if ( pause_on_recs > 0 ) m_pauseOnNrRecordings = pause_on_recs;

  int switch_min_before = q.getBodyAsInt("switch_min_before");
  if ( switch_min_before >= 0 ) m_switchMinBefore = switch_min_before;
 
  int marginstart = q.getBodyAsInt("margin_start");
  int marginstop = q.getBodyAsInt("margin_stop");
  if (marginstart >= 0) m_marginstart = marginstart;
  if (marginstop >= 0) m_marginstop = marginstop;
  
  m_useVPS = q.getBodyAsBool("use_vps");
  m_avoidrepeats = q.getBodyAsBool("avoid_repeats");
  
  if (m_avoidrepeats) {
     int repeats = q.getBodyAsInt("allowed_repeats");
     if (repeats < 0) return "allowed_repeats invalid";
     m_allowedrepeats = repeats;
  }
  
  m_compareTitle = q.getBodyAsBool("compare_title");
  
  int compareSubtitle = q.getBodyAsInt("compare_subtitle");
  if (compareSubtitle > 0 ) m_compareSubtitle = compareSubtitle;
  
  m_compareSummary = q.getBodyAsBool("compare_summary");
 
  int repeatsWithinDays = q.getBodyAsInt("repeats_within_days");
  if (repeatsWithinDays > 0) m_repeatsWithinDays = repeatsWithinDays;

  int compareCategories = q.getBodyAsInt("compare_categories");
  if (compareCategories >= 0) { m_catvaluesAvoidRepeat = compareCategories; }

  int del_mode = q.getBodyAsInt("del_mode");
  if (del_mode >= 0) m_delMode = del_mode;
  
  int delAfterCountRecs = q.getBodyAsInt("del_after_count_recs");
  if (delAfterCountRecs >= 0) m_delAfterCountRecs = delAfterCountRecs;

  int delAfterDaysOfFirstRec = q.getBodyAsInt("del_after_days_of_first_rec");
  if (delAfterDaysOfFirstRec >= 0) m_delAfterDaysOfFirstRec = delAfterDaysOfFirstRec;

  m_unmuteSoundOnSwitch = q.getBodyAsBool("unmute_sound_on_switch");

  m_compareTime = q.getBodyAsInt("compare_time");
  if (m_compareTime < 0 || m_compareTime > 3) {
      m_compareTime = 0;
  }
  m_summaryMatch = q.getBodyAsInt("summary_match");
  if (m_summaryMatch < 0 || m_summaryMatch > 100) {
      m_summaryMatch = 90;
  }

  m_catvaluesAvoidRepeat = q.getBodyAsInt("compare_categories");

  size_t max = 0; int t=1;
  ExtEPGInfos extepginfos;
  size_t i;
  for (i=0; i < extepginfos.size();i++) {
      max = max | t;
      t*=2;
  }
  if (m_catvaluesAvoidRepeat < 0 || m_catvaluesAvoidRepeat > max) {
      m_catvaluesAvoidRepeat = 0;
  }

  return ""; 
} 

istream& operator>>( istream& is, tChannelID& ret )
{
  string line;
  if (!getline( is, line ) ) {
    if (0 == is.gcount()) {
      is.clear(is.rdstate() & ~ios::failbit);
      return is;
    }
    if (!is.eof()) {
      is.setstate( ios::badbit );
      return is;
    }
  }

  if ( !line.empty() && !( ret = tChannelID::FromString( line.c_str() ) ).Valid() )
    is.setstate( ios::badbit );
  return is;
}

template< typename To, typename From >
To lexical_cast( From const& from )
{
   stringstream parser;
   parser << from;
   To result;
   parser >> result;
   if ( !parser )
      throw bad_lexical_cast();
   return result;
}

static char ServiceInterface[] = "Epgsearch-services-v1.0";

bool operator<( SearchTimer const& left, SearchTimer const& right )
{
   string leftlower = left.m_search;
   string rightlower = right.m_search;
   transform(leftlower.begin(), leftlower.end(), leftlower.begin(), (int(*)(int)) tolower);
   transform(rightlower.begin(), rightlower.end(), rightlower.begin(), (int(*)(int)) tolower);
   return leftlower < rightlower;
}

SearchTimer::SearchTimer()
{
   Init();
}

void SearchTimer::Init()
{
	m_id = -1;
	m_useTime = false;
	m_startTime = 0;
	m_stopTime = 0;
	m_useChannel = NoChannel;
	m_useCase = false;
	m_mode = 0;
	m_useTitle = true;
	m_useSubtitle = true;
	m_useDescription = true;
	m_useDuration = false;
	m_minDuration = 0;
	m_maxDuration = 0;
	m_useDayOfWeek = false;
	m_dayOfWeek = 0;
	m_useEpisode = false;
	m_priority = 50;
	m_lifetime = 99;
	m_fuzzytolerance = 1;
	m_useInFavorites = false;
	m_useAsSearchtimer = 0;
	m_action = 0;
	m_delAfterDays = 0;
	m_recordingsKeep = 0;
	m_pauseOnNrRecordings = 0;
	m_switchMinBefore = 1;
	m_useExtEPGInfo = false;
	m_useVPS = false;
	m_marginstart = 10; //configuration possibility?
	m_marginstop = 5; //configuration possibility?
	m_avoidrepeats = false;
	m_allowedrepeats = 0;
	m_compareTitle = false;
	m_compareSubtitle = 0;
	m_compareSummary = false;
	m_repeatsWithinDays = 0;
	m_blacklistmode = 0;
	m_menuTemplate = 0;
	m_delMode = 0;
	m_delAfterCountRecs = 0;
	m_delAfterDaysOfFirstRec = 0;
	m_useAsSearchTimerFrom = 0;
	m_useAsSearchTimerTil = 0;	
	m_catvaluesAvoidRepeat = 0;
	m_ignoreMissingEPGCats = false;
	m_unmuteSoundOnSwitch = false;
	m_summaryMatch = 90;
	m_contentDescriptors = "";
	m_compareTime = 0;
}

SearchTimer::SearchTimer( string const& data )
{
   Init();
   vector< string > parts = StringExtension::split( data, ":" );
   try {
      vector< string >::const_iterator part = parts.begin();
      for ( int i = 0; part != parts.end(); ++i, ++part ) {
			switch ( i ) {
			case  0: m_id = lexical_cast< int >( *part ); break;
			case  1: m_search = StringExtension::replace( StringExtension::replace( *part, "|", ":" ), "!^pipe^!", "|" ); break;
			case  2: m_useTime = lexical_cast< bool >( *part ); break;
			case  3: if ( m_useTime ) m_startTime = lexical_cast< int >( *part ); break;
			case  4: if ( m_useTime ) m_stopTime = lexical_cast< int >( *part ); break;
			case  5: m_useChannel = lexical_cast< int >( *part ); break;
			case  6: ParseChannel( *part ); break;
			case  7: m_useCase = lexical_cast< int >( *part ); break;
			case  8: m_mode = lexical_cast< int >( *part ); break;
			case  9: m_useTitle = lexical_cast< bool >( *part ); break;
			case 10: m_useSubtitle = lexical_cast< bool >( *part ); break;
			case 11: m_useDescription = lexical_cast< bool >( *part ); break;
			case 12: m_useDuration = lexical_cast< bool >( *part ); break;
			case 13: if ( m_useDuration ) m_minDuration = lexical_cast< int >( *part ); break;
			case 14: if ( m_useDuration ) m_maxDuration = lexical_cast< int >( *part ); break;
			case 15: m_useAsSearchtimer = lexical_cast< int >( *part ); break;
			case 16: m_useDayOfWeek = lexical_cast< bool >( *part ); break;
			case 17: m_dayOfWeek = lexical_cast< int >( *part ); break;
			case 18: m_useEpisode = lexical_cast< bool >( *part ); break;
			case 19: m_directory = StringExtension::replace( StringExtension::replace( *part, "|", ":" ), "!^pipe^!", "|" ); break;
			case 20: m_priority = lexical_cast< int >( *part ); break;
			case 21: m_lifetime = lexical_cast< int >( *part ); break;
			case 22: m_marginstart = lexical_cast< int >( *part ); break;
			case 23: m_marginstop = lexical_cast< int >( *part ); break;
			case 24: m_useVPS = lexical_cast< bool >( *part ); break;
			case 25: m_action = lexical_cast< int >( *part ); break;
			case 26: m_useExtEPGInfo = lexical_cast< bool >( *part ); break;
			case 27: ParseExtEPGInfo( *part ); break;
			case 28: m_avoidrepeats = lexical_cast< bool >( *part ); break;
			case 29: m_allowedrepeats = lexical_cast< int >( *part ); break;
			case 30: m_compareTitle = lexical_cast< bool >( *part ); break;
			case 31: m_compareSubtitle = lexical_cast< int >( *part ); break;
			case 32: m_compareSummary = lexical_cast< bool >( *part ); break;
			case 33: m_catvaluesAvoidRepeat = lexical_cast< long >( *part ); break;
			case 34: m_repeatsWithinDays = lexical_cast< int >( *part ); break;
			case 35: m_delAfterDays = lexical_cast< int >( *part ); break;
			case 36: m_recordingsKeep = lexical_cast< int >( *part ); break;
			case 37: m_switchMinBefore = lexical_cast< int >( *part ); break;
			case 38: m_pauseOnNrRecordings = lexical_cast< int >( *part ); break;
			case 39: m_blacklistmode = lexical_cast< int >( *part ); break;
			case 40: ParseBlacklist( *part ); break;
			case 41: m_fuzzytolerance = lexical_cast< int >( *part ); break;
			case 42: m_useInFavorites = lexical_cast< bool >( *part ); break;
			case 43: m_menuTemplate = lexical_cast< int >( *part ); break;
			case 44: m_delMode = lexical_cast< int >( *part ); break;
			case 45: m_delAfterCountRecs = lexical_cast< int >( *part ); break;
			case 46: m_delAfterDaysOfFirstRec = lexical_cast< int >( *part ); break;
			case 47: m_useAsSearchTimerFrom = lexical_cast< time_t >( *part ); break;
			case 48: m_useAsSearchTimerTil = lexical_cast< time_t >( *part ); break;
			case 49: m_ignoreMissingEPGCats = lexical_cast< bool >( *part ); break;
			case 50: m_unmuteSoundOnSwitch = lexical_cast< bool >( *part ); break;
			case 51: m_summaryMatch = lexical_cast< int >( *part ); break;
			case 52: m_contentDescriptors =  *part; break;
			case 53: m_compareTime = lexical_cast< int >( *part ); break;

			}
		}
	} catch ( bad_lexical_cast const& ex ) {
	}
}

string SearchTimer::ToText()
{
   string tmp_Start;
   string tmp_Stop;
   string tmp_minDuration;
   string tmp_maxDuration;
   string tmp_chanSel;
   string tmp_search;
   string tmp_directory;
   string tmp_catvalues;
   string tmp_blacklists;

   tmp_search = StringExtension::replace(StringExtension::replace(m_search, "|", "!^pipe^!"), ":", "|");
   tmp_directory = StringExtension::replace(StringExtension::replace(m_directory, "|", "!^pipe^!"), ":", "|");

   if (m_useTime)
   {
      ostringstream os;
      os << setw(4) << setfill('0') << m_startTime;
      tmp_Start = os.str();
      os.str("");
      os << setw(4) << setfill('0') << m_stopTime;
      tmp_Stop = os.str();
   }
   if (m_useDuration)
   {
      ostringstream os;
      os << setw(4) << setfill('0') << m_minDuration;
      tmp_minDuration = os.str();
      os.str("");
      os << setw(4) << setfill('0') << m_maxDuration;
      tmp_maxDuration = os.str();
   }

   if (m_useChannel==1) {

      cChannel const* channelMin = Channels.GetByChannelID( m_channelMin );
      cChannel const* channelMax = Channels.GetByChannelID( m_channelMax );

      if (channelMax && channelMin->Number() < channelMax->Number())
         tmp_chanSel = *m_channelMin.ToString() + string("|") + *m_channelMax.ToString();
      else
         tmp_chanSel = *m_channelMin.ToString();
   }
   if (m_useChannel==2)
      tmp_chanSel = m_channels;

   if (m_useExtEPGInfo)
   {
      for(unsigned int i=0; i<m_ExtEPGInfo.size(); i++) {
         tmp_catvalues += (tmp_catvalues != ""?"|":"") + 
            StringExtension::replace(StringExtension::replace(m_ExtEPGInfo[i], ":", "!^colon^!"), "|", "!^pipe^!");
      }
   }

   if (m_blacklistmode == 1)
   {
      for(unsigned int i=0; i<m_blacklistIDs.size(); i++) {
	 string blacklist_id = StringExtension::toString(itoa(m_blacklistIDs[i]));
         tmp_blacklists += (tmp_blacklists != ""?"|":"") + blacklist_id;
      }
   }

   ostringstream os;
   os << m_id << ":"
      << tmp_search << ":"
      << (m_useTime?1:0) << ":"
      << tmp_Start << ":"
      << tmp_Stop << ":"
      << m_useChannel << ":"
      << ((m_useChannel>0 && m_useChannel<3)?tmp_chanSel:"0") << ":"
      << (m_useCase?1:0) << ":"
      << m_mode << ":"
      << (m_useTitle?1:0) << ":"
      << (m_useSubtitle?1:0) << ":"
      << (m_useDescription?1:0) << ":"
      << (m_useDuration?1:0) << ":"
      << tmp_minDuration << ":"
      << tmp_maxDuration << ":"
      << m_useAsSearchtimer << ":"
      << (m_useDayOfWeek?1:0) << ":"
      << m_dayOfWeek << ":"
      << (m_useEpisode?1:0) << ":"
      << tmp_directory << ":"
      << m_priority << ":"
      << m_lifetime << ":"
      << m_marginstart << ":"
      << m_marginstop << ":"
      << (m_useVPS?1:0) << ":"
      << m_action << ":"
      << (m_useExtEPGInfo?1:0) << ":"
      << tmp_catvalues << ":"
      << (m_avoidrepeats?1:0) << ":"
      << m_allowedrepeats << ":"
      << (m_compareTitle?1:0) << ":"
      << m_compareSubtitle << ":"
      << (m_compareSummary?1:0) << ":"
      << m_catvaluesAvoidRepeat << ":"
      << m_repeatsWithinDays << ":"
      << m_delAfterDays << ":"
      << m_recordingsKeep << ":"
      <<  m_switchMinBefore << ":"
      << m_pauseOnNrRecordings << ":"
      << m_blacklistmode << ":"
      << tmp_blacklists << ":"
      << m_fuzzytolerance << ":"
      << (m_useInFavorites?1:0) << ":"
      << m_menuTemplate << ":"
      << m_delMode << ":"
      << m_delAfterCountRecs << ":"
      << m_delAfterDaysOfFirstRec << ":"
      << (long) m_useAsSearchTimerFrom << ":"
      << (long) m_useAsSearchTimerTil << ":"
      << m_ignoreMissingEPGCats << ":"
      << m_unmuteSoundOnSwitch << ":"
      << m_summaryMatch << ":"
      << m_contentDescriptors << ":"
      << m_compareTime;

   return os.str();
}

void SearchTimer::ParseChannel( string const& data )
{
	switch ( m_useChannel ) {
	case NoChannel: m_channels = tr("All"); break;
	case Interval: ParseChannelIDs( data ); break;
	case Group: m_channels = data; break;
	case FTAOnly: m_channels = tr("FTA"); break;
	}
}

void SearchTimer::ParseChannelIDs( string const& data )
{
	vector< string > parts = StringExtension::split( data, "|" );
	m_channelMin = lexical_cast< tChannelID >( parts[ 0 ] );

	cChannel const* channel = Channels.GetByChannelID( m_channelMin );
	if ( channel != 0 )
		m_channels = channel->Name();

	if ( parts.size() < 2 )
		return;

	m_channelMax = lexical_cast< tChannelID >( parts[ 1 ] );

	channel = Channels.GetByChannelID( m_channelMax );
	if ( channel != 0 )
		m_channels += string( " - " ) + channel->Name();
}

void SearchTimer::ParseExtEPGInfo( string const& data )
{
   m_ExtEPGInfo = StringExtension::split( data, "|" );
}

void SearchTimer::ParseBlacklist( string const& data )
{
  vector< string > ids = StringExtension::split( data, "|" );

  vector< string >::const_iterator id = ids.begin();
  for ( int i = 0; id != ids.end(); ++i, ++id ) {

      m_blacklistIDs.push_back(atoi(ids[i].c_str()));
  }
}

SearchTimers::SearchTimers()
{
	Reload();
}

bool SearchTimers::Reload()
{
	Epgsearch_services_v1_0 service;
        cPluginManager::CallFirstService(ServiceInterface, &service);
	ReadLock channelsLock( Channels, 0 );
	list< string > timers = service.handler->SearchTimerList();
	m_timers.assign( timers.begin(), timers.end() );
	m_timers.sort();
	return true;
}

bool SearchTimers::Save(SearchTimer* searchtimer)
{
	Epgsearch_services_v1_0 service;
	cPluginManager::CallFirstService(ServiceInterface, &service);

	if (!searchtimer) return false;
	ReadLock channelsLock( Channels, 0 );
	if (searchtimer->Id() >= 0)
		return service.handler->ModSearchTimer(searchtimer->ToText());
	else
	{
		searchtimer->SetId(0);
		int id = service.handler->AddSearchTimer(searchtimer->ToText());
		if (id >= 0)
			searchtimer->SetId(id);
		return (id >= 0);
	}
}

SearchTimer* SearchTimers::GetByTimerId( string const& id )
{
   for (SearchTimers::iterator timer = m_timers.begin(); timer != m_timers.end(); ++timer) 
      if (timer->Id() == lexical_cast< int >(id)) 
         return &*timer;
   return NULL;
      
}

bool SearchTimers::ToggleActive(string const& id)
{
	SearchTimer* search = GetByTimerId( id );
	if (!search) return false;
	search->SetUseAsSearchTimer(search->UseAsSearchTimer()==1?0:1);
	return Save(search);
}

bool SearchTimers::Delete(string const& id)
{
	SearchTimer* search = GetByTimerId( id );
	if (!search) return false;

	Epgsearch_services_v1_0 service;
	cPluginManager::CallFirstService(ServiceInterface, &service);

	if (service.handler->DelSearchTimer(lexical_cast< int >( id )))
		return Reload();
	return false;
}

void SearchTimers::TriggerUpdate()
{
	Epgsearch_updatesearchtimers_v1_0 service;
	service.showMessage = true;
	cPluginManager::CallFirstService("Epgsearch-updatesearchtimers-v1.0", &service);
}

bool SearchTimer::BlacklistSelected(int id) const
{ 
   for(unsigned int i=0; i<m_blacklistIDs.size(); i++) 
      if (m_blacklistIDs[i] == id) return true;
   return false; 
}

ExtEPGInfo::ExtEPGInfo( string const& data )
{
   m_id = -1;
   m_searchmode = 0;
   m_data = data;

   vector< string > parts = StringExtension::split( data, "|" );
   try {
      vector< string >::const_iterator part = parts.begin();
      for ( int i = 0; part != parts.end(); ++i, ++part ) {
         switch ( i ) {
			case  0: m_id = lexical_cast< int >( *part ); break;
			case  1: m_name = *part; break;
			case  2: m_menuname = *part; break;
			case  3: ParseValues( *part ); break;
			case  4: m_searchmode = lexical_cast< int >( *part ); break;
         }
      }
   } catch ( bad_lexical_cast const& ex ) {
   }
}

void ExtEPGInfo::ParseValues( string const& data )
{
   m_values = StringExtension::split( data, "," );

   for(std::vector<int>::size_type i = 0; i != m_values.size(); i++) {
       m_values[i] = StringExtension::trim(m_values[i]);
   }
}

bool ExtEPGInfo::Selected(unsigned int index, string const& values)
{
   if (index >= m_values.size()) return false;
   string extepgvalue = StringExtension::trim(m_values[index]);

   vector< string > parts;
   parts = StringExtension::split( values, "," );
   for(unsigned int i=0; i<parts.size(); i++) if (StringExtension::trim(parts[i]) == extepgvalue) return true; 
   parts = StringExtension::split( values, ";" );
   for(unsigned int i=0; i<parts.size(); i++) if (StringExtension::trim(parts[i]) == extepgvalue) return true; 
   parts = StringExtension::split( values, "|" );
   for(unsigned int i=0; i<parts.size(); i++) if (StringExtension::trim(parts[i]) == extepgvalue) return true; 
   parts = StringExtension::split( values, "~" );
   for(unsigned int i=0; i<parts.size(); i++) if (StringExtension::trim(parts[i]) == extepgvalue) return true; 
   return false;
}

ExtEPGInfos::ExtEPGInfos()
{
	Epgsearch_services_v1_0 service;
	cPluginManager::CallFirstService(ServiceInterface, &service);

	list< string > infos = service.handler->ExtEPGInfoList();
	m_infos.assign( infos.begin(), infos.end() );
}

ChannelGroup::ChannelGroup( string const& data )
{
   vector< string > parts = StringExtension::split( data, "|" );

   // TODO: check what else is provided in data -> Channel ids -> epgsearch accepts *char channelgroupname

   try {
      vector< string >::const_iterator part = parts.begin();
      for ( int i = 0; part != parts.end(); ++i, ++part ) {
         switch ( i ) {
           case  0: m_name = *part; break;
         }
      }
   } catch ( bad_lexical_cast const& ex ) {
   }
}

ChannelGroups::ChannelGroups()
{
   Epgsearch_services_v1_0 service;
   cPluginManager::CallFirstService(ServiceInterface, &service);

   list< string > list = service.handler->ChanGrpList();
   m_list.assign( list.begin(), list.end() );
}

Blacklist::Blacklist( string const& data )
{
   vector< string > parts = StringExtension::split( data, ":" );
   try {
      vector< string >::const_iterator part = parts.begin();
      for ( int i = 0; part != parts.end(); ++i, ++part ) {
			switch ( i ) {
			case  0: m_id = lexical_cast< int >( *part ); break;
			case  1: m_search = StringExtension::replace( StringExtension::replace( *part, "|", ":" ), "!^pipe^!", "|" ); break;
			}
		}
	} catch ( bad_lexical_cast const& ex ) {
	}
}

Blacklists::Blacklists()
{
   Epgsearch_services_v1_0 service;
   cPluginManager::CallFirstService(ServiceInterface, &service);
   list< string > list = service.handler->BlackList();
   m_list.assign( list.begin(), list.end() );
   m_list.sort();
}

SearchResult::SearchResult( string const& data )
{
	vector< string > parts = StringExtension::split( data, ":" );
	try {
		vector< string >::const_iterator part = parts.begin();
		for ( int i = 0; part != parts.end(); ++i, ++part ) {
			switch ( i ) {
			case  0: m_searchId = lexical_cast< int >( *part ); break;
			case  1: m_eventId = lexical_cast< u_int32_t >( *part ); break;
			case  2: m_title = StringExtension::replace( *part, "|", ":" ); break;
			case  3: m_shorttext = StringExtension::replace( *part, "|", ":" ); break;
			case  4: m_description = StringExtension::replace( *part, "|", ":" ); break;
			case  5: m_starttime = lexical_cast< unsigned long >( *part ); break;
			case  6: m_stoptime = lexical_cast< unsigned long >( *part ); break;
			case  7: m_channel = lexical_cast< tChannelID >( *part ); break;
			case  8: m_timerstart = lexical_cast< unsigned long >( *part ); break;
			case  9: m_timerstop = lexical_cast< unsigned long >( *part ); break;
			case 10: m_file = *part; break;
			case 11: m_timerMode = lexical_cast< int >( *part ); break;
			}
		}
	} catch ( bad_lexical_cast const& ex ) {
	}
}

const cEvent* SearchResult::GetEvent()
{
	cSchedulesLock schedulesLock;
	const cSchedules* Schedules = cSchedules::Schedules(schedulesLock);
	if (!Schedules) return NULL;
	const cChannel *Channel = GetChannel();
	if (!Channel) return NULL;
	const cSchedule *Schedule = Schedules->GetSchedule(Channel);
	if (!Schedule) return NULL;
	return Schedule->GetEvent(m_eventId);	
}

const cChannel* SearchResult::GetChannel() {

#if APIVERSNUM > 20300
    LOCK_CHANNELS_READ;
    const cChannels& channels = *Channels;
#else
    cChannels& channels = Channels;
#endif

    return channels.GetByChannelID(m_channel);
}

set<string> SearchResults::querySet;

void SearchResults::GetByID(int id)
{
   Epgsearch_services_v1_0 service;
   cPluginManager::CallFirstService(ServiceInterface, &service);

   list< string > list = service.handler->QuerySearchTimer(id);
   m_list.assign( list.begin(), list.end() );
   m_list.sort();
}

void SearchResults::GetByQuery(string const& query)
{
   Epgsearch_services_v1_0 service;
   cPluginManager::CallFirstService(ServiceInterface, &service); 

   list< string > list = service.handler->QuerySearch(query);
   m_list.assign( list.begin(), list.end() );
   m_list.sort();
}

RecordingDirs::RecordingDirs(bool shortList)
{
  if (shortList)
    {
      Epgsearch_services_v1_2 service;
      cPluginManager::CallFirstService(ServiceInterface, &service);
      
      m_set = service.handler->ShortDirectoryList();
    }
  else
    {	
      Epgsearch_services_v1_0 service;
      cPluginManager::CallFirstService(ServiceInterface, &service);
      
      m_set = service.handler->DirectoryList();
    }
}

string EPGSearchSetupValues::ReadValue(const string& entry)
{
   Epgsearch_services_v1_0 service;
   cPluginManager::CallFirstService(ServiceInterface, &service);
   return service.handler->ReadSetupValue(entry);
}

bool EPGSearchSetupValues::WriteValue(const string& entry, const string& value)
{
   Epgsearch_services_v1_0 service;
   cPluginManager::CallFirstService(ServiceInterface, &service);

   return service.handler->WriteSetupValue(entry, value);
}

string EPGSearchExpr::EvaluateExpr(const string& expr, const cEvent* event)
{
   Epgsearch_services_v1_2 service;
   cPluginManager::CallFirstService(ServiceInterface, &service);
   return service.handler->Evaluate(expr, event);
}

TimerConflicts::TimerConflicts() {

  Epgsearch_services_v1_1 service;
  cPluginManager::CallFirstService(ServiceInterface, &service);

  m_list = service.handler->TimerConflictList();
  m_checkAdvised = service.handler->IsConflictCheckAdvised();
}

} // namespace vdrlive

