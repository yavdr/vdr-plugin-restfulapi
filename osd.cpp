#include "osd.h"
using namespace std;

void OsdResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/osd", request);

  if ( request.method() != "GET" ) {
     reply.httpReturn(403, "Only GET-method is supported!");
     return;
  }

  BasicOsd* osd = StatusMonitor::get()->getOsd();

  if ( osd == NULL ) {
     if ( q.isFormat(".html") ) {
        reply.addHeader("Content-Type", "text /html; charset=utf-8");
        printEmptyHtml(out);
        return;
     } else {
        reply.httpReturn(404, "No OSD opened!");
        return;
     }
  }

  string format = "";
  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     format = ".json";
  } else if ( q.isFormat(".html") ) {
     format = ".html";
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     format = ".xml";
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .html or .xml)");
     return;
  }

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");

  switch(osd->Type())
  {
     case 0x01: printTextOsd(out, (TextOsd*)osd, format, start_filter, limit_filter);
                break;
     case 0x02: { ChannelOsdWrapper* w = new ChannelOsdWrapper(&out);
                  w->print((ChannelOsd*)osd, format);
                  delete w; }
                break;
     case 0x03: { ProgrammeOsdWrapper* w = new ProgrammeOsdWrapper(&out);
                  w->print((ProgrammeOsd*)osd, format);
                  delete w; }
                break;
  }
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTextOsdItem& o)
{
  si.addMember("content") <<= o.Content;
  si.addMember("is_selected") <<= o.Selected;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTextOsd& o)
{
  si.addMember("type") <<= "TextOsd";
  si.addMember("title") <<= o.Title;
  si.addMember("message") <<= o.Message;
  si.addMember("red") <<= o.Red;
  si.addMember("green") <<= o.Green;
  si.addMember("yellow") <<= o.Yellow;
  si.addMember("blue") <<= o.Blue;

  si.addMember("items") <<= o.ItemContainer->items;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerProgrammeOsd& o)
{
  si.addMember("present_time") <<= o.PresentTime;
  si.addMember("present_title") <<= o.PresentTitle;
  si.addMember("present_subtitle") <<= o.PresentSubtitle;
  si.addMember("following_time") <<= o.FollowingTime;
  si.addMember("following_title") <<= o.FollowingTitle;
  si.addMember("following_subtitle") <<= o.FollowingSubtitle;
}

void OsdResponder::printEmptyHtml(ostream& out)
{
  StreamExtension se(&out);

  HtmlHeader htmlHeader;
  htmlHeader.Title("VDR Restfulapi: No OSD opened!");
  htmlHeader.Stylesheet("/var/lib/vdr/plugins/restfulapi/osd.css");
  htmlHeader.Script("/var/lib/vdr/plugins/restfulapi/osd.js");
  htmlHeader.MetaTag("<meta http-equiv=\"refresh\" content=\"1\" />");
  htmlHeader.ToStream(&se);

  se.write("\n<div id=\"osd_bg\"><div id=\"osd_container\">&nbsp;</div></div>\n");
  se.write("</body></html>");
}

void OsdResponder::printTextOsd(ostream& out, TextOsd* osd, string format, int start_filter, int limit_filter)
{
  TextOsdList* osdList = NULL;

  if ( format == ".json" ) {
     osdList = (TextOsdList*)new JsonTextOsdList(&out);
  } else if ( format == ".xml" ) {
     osdList = (TextOsdList*)new XmlTextOsdList(&out);
  } else if ( format == ".html" ) {
     osdList = (TextOsdList*)new HtmlTextOsdList(&out);
  }

  if ( osdList != NULL ) {
     if (start_filter >= 0 && limit_filter >= 1 ) {
        osdList->activateLimit(start_filter, limit_filter);
     }
     osdList->printTextOsd(osd);
  }
}

// --- XmlTextOsdList --------------------------------------------------------------------------------

void XmlTextOsdList::printTextOsd(TextOsd* osd)
{
  s->writeXmlHeader();
  s->write("<TextOsd xmlns=\"http://www.domain.org/restfulapi/2011/TextOsd-xml\">\n");
  s->write(cString::sprintf(" <title>%s</title>\n", StringExtension::encodeToXml(osd->Title()).c_str()));
  s->write(cString::sprintf(" <message>%s</message>\n", StringExtension::encodeToXml(osd->Message()).c_str()));
  s->write(cString::sprintf(" <red>%s</red>\n", StringExtension::encodeToXml(osd->Red()).c_str()));
  s->write(cString::sprintf(" <green>%s</green>\n", StringExtension::encodeToXml(osd->Green()).c_str()));
  s->write(cString::sprintf(" <yellow>%s</yellow>\n", StringExtension::encodeToXml(osd->Yellow()).c_str()));
  s->write(cString::sprintf(" <blue>%s</blue>\n", StringExtension::encodeToXml(osd->Blue()).c_str()));

  list<TextOsdItem*>::iterator it;
  list<TextOsdItem*> items = osd->GetItems();

  s->write(" <items>\n");
  for( it = items.begin(); it != items.end(); ++it ) {
    if (!filtered()) {
       const char* selected = (*it) == osd->Selected() ? "true" : "false";
       TextOsdItem* item = *it;
       s->write(cString::sprintf(" <item selected=\"%s\">%s</item>\n", selected, StringExtension::encodeToXml(item->Text()).c_str()));
    }
  }
  s->write(" </items>\n");
  s->write("</TextOsd>\n");
}

// --- JsonTextOsdList -------------------------------------------------------------------------------

void JsonTextOsdList::printTextOsd(TextOsd* textOsd)
{
  SerTextOsd t;

  t.Title = StringExtension::UTF8Decode(textOsd->Title());
  t.Message = StringExtension::UTF8Decode(textOsd->Message());
  t.Red = StringExtension::UTF8Decode(textOsd->Red());
  t.Green = StringExtension::UTF8Decode(textOsd->Green());
  t.Yellow = StringExtension::UTF8Decode(textOsd->Yellow());
  t.Blue = StringExtension::UTF8Decode(textOsd->Blue());

  SerTextOsdItemContainer* itemContainer = new SerTextOsdItemContainer();

  list<TextOsdItem*>::iterator it;
  list<TextOsdItem*> items = textOsd->GetItems();

  for(it = items.begin(); it != items.end(); ++it)
  {
    if (!filtered()) {
       SerTextOsdItem sitem;
       sitem.Content = cxxtools::String(StringExtension::UTF8Decode((*it)->Text()));
       sitem.Selected = (*it) == textOsd->Selected() ? true : false;
       itemContainer->items.push_back(sitem);
    }
  }

  t.ItemContainer = itemContainer;;

  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(t, "TextOsd");
  serializer.finish();

  //delete itemsArray;
  delete itemContainer;
}

// --- HtmlTextOsdList -------------------------------------------------------------------------------

void HtmlTextOsdList::printTextOsd(TextOsd* textOsd)
{
  HtmlHeader htmlHeader;
  htmlHeader.Title("HtmlTextOsdList");
  htmlHeader.Stylesheet("/var/lib/vdr/plugins/restfulapi/osd.css");
  htmlHeader.Script("/var/lib/vdr/plugins/restfulapi/osd.js");
  htmlHeader.MetaTag("<meta http-equiv=\"refresh\" content=\"1\" />");
  htmlHeader.ToStream(s);

  s->write("\n<div id=\"osd_bg\"><div id=\"osd_container\">");
  s->write("\n<div id=\"header\">");
    if (textOsd->Title().length() > 0)
       s->write(textOsd->Title().c_str());
    if (textOsd->Message().length() > 0) {
       s->write("\n");
       s->write(textOsd->Message().c_str());
    }

  s->write("</div><!-- closing header container -->\n");

  s->write("<div id=\"content\">\n");
    list< TextOsdItem* > items = textOsd->GetItems();
    list< TextOsdItem* >::iterator it;
    s->write("<ul type=\"none\">\n");
    for(it = items.begin(); it != items.end(); ++it) {
       if (!filtered()) {
          s->write("<li class=\"item\"");
          s->write((*it) == textOsd->Selected() ? " id=\"selectedItem\">" : ">" );
          s->write((*it)->Text().c_str());
          s->write("</li>\n");
       }
    }
  s->write("</ul>\n");
  s->write("</div><!-- closing content container -->\n");

  s->write("<div id=\"color_buttons\">\n");
  if (textOsd->Red().length() > 0)
     s->write(cString::sprintf("<div id=\"red\" class=\"first active\">%s</div>\n", textOsd->Red().c_str()));
  else
     s->write("<div id=\"red\" class=\"first inactive\">&nbsp;</div>\n");

  if (textOsd->Green().length() > 0)
     s->write(cString::sprintf("<div id=\"green\" class=\"second active\">%s</div>\n", textOsd->Green().c_str()));
  else
     s->write("<div id=\"green\" class=\"second inactive\">&nbsp;</div>\n");

  if (textOsd->Yellow().length() > 0)
     s->write(cString::sprintf("<div id=\"yellow\" class=\"third active\">%s</div>\n", textOsd->Yellow().c_str()));
  else
     s->write("<div id=\"yellow\" class=\"third inactive\">&nbsp;</div>\n");

  if (textOsd->Blue().length() > 0)
     s->write(cString::sprintf("<div id=\"blue\" class=\"fourth active\">%s</div>\n", textOsd->Blue().c_str()));
  else
     s->write("<div id=\"blue\" class=\"fourth inactive\">&nbsp;</div>\n");

  s->write("<br class=\"clear\">\n</div><!-- closing color_buttons container -->\n");
  s->write("</div></div><!-- closing osd_container -->\n");
  s->write("</body></html>");
}

// --- ProgrammeOsdWrapper ---------------------------------------------------------------------------

void ProgrammeOsdWrapper::print(ProgrammeOsd* osd, string format)
{
  if ( format == ".json" ) {
     printJson(osd);
  } else if ( format == ".html" ) {
     printHtml(osd);
  } else if ( format == ".xml") {
     printXml(osd);
  }
}

void ProgrammeOsdWrapper::printXml(ProgrammeOsd* osd)
{
  s->writeXmlHeader();
  s->write("<ProgrammeOsd xmlns=\"http://www.domain.org/restfulapi/2011/ProgrammeOsd-xml\">\n");
  s->write(cString::sprintf(" <presenttime>%i</presenttime>\n", (int)osd->PresentTime()));
  s->write(cString::sprintf(" <presenttitle>%s</presenttitle>\n", StringExtension::encodeToXml(osd->PresentTitle()).c_str()));
  s->write(cString::sprintf(" <presentsubtitle>%s</presentsubtitle>\n", StringExtension::encodeToXml(osd->PresentSubtitle()).c_str()));
  s->write(cString::sprintf(" <followingtime>%i</followingtime>\n", (int)osd->FollowingTime()));
  s->write(cString::sprintf(" <followingtitle>%s</followingtitle>\n", StringExtension::encodeToXml(osd->FollowingTitle()).c_str()));
  s->write(cString::sprintf(" <followingsubtitle>%s</followingsubtitle>\n", StringExtension::encodeToXml(osd->FollowingSubtitle()).c_str()));
  s->write("</ProgrammeOsd>\n");
}

void ProgrammeOsdWrapper::printJson(ProgrammeOsd* osd)
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  SerProgrammeOsd p;
  p.PresentTime = osd->PresentTime();
  p.PresentTitle = StringExtension::UTF8Decode(osd->PresentTitle());
  p.PresentSubtitle = StringExtension::UTF8Decode(osd->PresentSubtitle());
  p.FollowingTime = osd->FollowingTime();
  p.FollowingTitle = StringExtension::UTF8Decode(osd->FollowingTitle());
  p.FollowingSubtitle = StringExtension::UTF8Decode(osd->FollowingSubtitle());
  serializer.serialize(p, "ProgrammeOsd");
  serializer.finish();
}

void ProgrammeOsdWrapper::printHtml(ProgrammeOsd* osd)
{
  HtmlHeader htmlHeader;
  htmlHeader.Title("ProgrammeOsdWrapper");
  htmlHeader.Stylesheet("/var/lib/vdr/plugins/restfulapi/osd.css");
  htmlHeader.Script("/var/lib/vdr/plugins/restfulapi/osd.js");
  htmlHeader.MetaTag("<meta http-equiv=\"refresh\" content=\"1\" />");
  htmlHeader.ToStream(s);
  s->write("<div id=\"osd_bg\"><div id=\"osd_container\">\n");
  s->write("<div id=\"content2\"><div id=\"innercontent\">");
  s->write(cString::sprintf("<div id=\"eventtitle\">%s</div>", osd->PresentTitle().c_str()));
  s->write(cString::sprintf("<div id=\"eventsubtitle\">%s - %s</div>",
osd->PresentSubtitle().c_str(),
StringExtension::timeToString(osd->PresentTime()).c_str()));
  s->write(cString::sprintf("<div id=\"eventtitle\">%s</div>", osd->FollowingTitle().c_str()));
  s->write(cString::sprintf("<div id=\"eventsubtitle\">%s - %s</div>",
osd->FollowingSubtitle().c_str(),
StringExtension::timeToString(osd->FollowingTime()).c_str()));
  s->write("</div></div>\n");
  s->write("</div></div>\n");
  s->write("</body></html>\n");
}

// --- ChannelOsdWrapper -----------------------------------------------------------------------------

void ChannelOsdWrapper::print(ChannelOsd* osd, string format)
{
  if ( format == ".json" ) {
     printJson(osd);
  } else if ( format == ".html" ) {
     printHtml(osd);
  } else if ( format == ".xml" ) {
     printXml(osd);
  }
}

void ChannelOsdWrapper::printXml(ChannelOsd* osd)
{
  s->writeXmlHeader();
  s->write("<ChannelOsd xmlns=\"http://www.domain.org/restfulapi/2011/ChannelOsd-xml\">\n");
  s->write(cString::sprintf(" <Text>%s</Text>\n", StringExtension::encodeToXml(osd->Channel()).c_str()));
  s->write("</ChannelOsd>\n");
}

void ChannelOsdWrapper::printJson(ChannelOsd* osd)
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(StringExtension::UTF8Decode(osd->Channel()), "ChannelOsd");
  serializer.finish();
}

void ChannelOsdWrapper::printHtml(ChannelOsd* osd)
{
  HtmlHeader htmlHeader;
  htmlHeader.Title("ChannelOsdWrapper");
  htmlHeader.Stylesheet("/var/lib/vdr/plugins/restfulapi/osd.css");
  htmlHeader.Script("/var/lib/vdr/plugins/restfulapi/osd.js");
  htmlHeader.MetaTag("<meta http-equiv=\"refresh\" content=\"1\" />");
  htmlHeader.ToStream(s);

  s->write("<div id=\"header\">");
  s->write(StringExtension::encodeToXml(osd->Channel()).c_str());
  s->write("</div>\n");
  s->write("</body></html>");
}
