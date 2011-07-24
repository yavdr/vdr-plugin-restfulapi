#include "jsonparser.h"

// --- JsonBase ----------------------------------------------------------

JsonBase::JsonBase(int type)
{
  _type = type;
}

JsonBase::~JsonBase()
{

}

bool JsonBase::IsObject()
{
  return _type == 0x01;
}

bool JsonBase::IsValue()
{
  return _type == 0x02;
}
 
bool JsonBase::IsBasicValue()
{
  return _type == 0x03;
}

bool JsonBase::IsArray()
{
  return _type == 0x04;
}

// --- JsonObject --------------------------------------------------------

JsonObject::JsonObject() : JsonBase(0x01)
{
  
}

JsonObject::~JsonObject()
{
  while(_items.size() != 0)
  {
    JsonBase* last = _items.back();
    _items.pop_back();
    delete last;
  }
}

void JsonObject::AddItem(JsonValue* item)
{
  _items.push_back(item);
}

JsonValue* JsonObject::GetItem(int i)
{
  if (i >= 0 && i < (int)_items.size()){
     return (JsonValue*)_items[i];
  }
  return NULL;
}

JsonValue* JsonObject::GetItem(std::string str)
{
  for(int i=0;i<(int)_items.size();i++) {
    if (_items[i]->IsValue()) {
       JsonValue* jsonValue = _items[i];
       if(jsonValue->Identifier() == str) {
          return jsonValue;
       }
    }
  }
  return NULL;
}

JsonValue* JsonObject::GetItem(const char* str)
{
  return GetItem((std::string)str);
}

int JsonObject::CountItem()
{
  return (int)_items.size();
}

// --- JsonValue ---------------------------------------------------------

JsonValue::JsonValue(std::string identifier) : JsonBase(0x02)
{
  _identifier = identifier;
  _value = NULL;
}

JsonValue::~JsonValue()
{
  if (_value != NULL)
  {
     delete _value;
  }
}

std::string JsonValue::Identifier()
{
  return _identifier;
}

void JsonValue::Value(JsonBase* value)
{
  _value = value;
}

JsonBase* JsonValue::Value()
{
  return _value;
}

// --- JsonBasicValue ----------------------------------------------------

JsonBasicValue::JsonBasicValue(std::string value) : JsonBase(0x03)
{
  _str = value;
  _type = 0x11;
}

JsonBasicValue::JsonBasicValue(double value) : JsonBase(0x03)
{
  _double = value;
  _type = 0x12;
}

JsonBasicValue::JsonBasicValue(bool value) : JsonBase(0x03)
{
  _bool = value;
  _type = 0x13;
}

JsonBasicValue::~JsonBasicValue()
{

}

bool JsonBasicValue::IsString()
{
  return _type == 0x11;
}

bool JsonBasicValue::IsBool()
{
  return _type == 0x13;
}

bool JsonBasicValue::IsDouble()
{
  return _type == 0x12;
}

std::string JsonBasicValue::ValueAsString()
{
  return _str;
}

double JsonBasicValue::ValueAsDouble()
{
  return _double;
}

bool JsonBasicValue::ValueAsBool()
{
  return _bool;
}

// --- JsonArray --------------------------------------------------------

JsonArray::JsonArray() : JsonBase(0x04)
{

}

JsonArray::~JsonArray()
{

}

void JsonArray::AddItem(JsonBase* item)
{
  _items.push_back(item);
}

JsonBase* JsonArray::GetItem(int i)
{
  if (i >= 0 && i < (int)_items.size())
  {
    return _items[i];
  }
  return NULL;
}

int JsonArray::CountItem()
{
  return (int)_items.size();
}

// --- JsonParser -------------------------------------------------------

JsonParser::JsonParser()
{
  
}

JsonParser::~JsonParser()
{

}

JsonObject* JsonParser::Parse(std::string str)
{
  long position = 0;
  return ParseJsonObject(str.c_str(), str.length(), &position);
}


bool JsonParser::SkipEmpty(const char* data, long size, long* position)
{
  while(*position < size) {
     if ( data[*position] != '\t' && 
          data[*position] != '\n' && 
          data[*position] != ' ' && 
          data[*position] != ':' && 
          data[*position] != ',' ) 
     {
        return true;
     } else {
        (*position)++;
     }
  }
  return false;
}

std::string JsonParser::ParseString(const char* data, long size, long* position)
{
  (*position)++; //ignore "
  bool escaped = false;
  std::ostringstream str;

  while(!(data[*position] == '"' && !escaped) && *position < size) {
     switch(data[*position]) {
       case '"': if(escaped) str << data[*position]; break;
       case '\\': if(escaped) str << '\\'; else escaped = true; break;
       default: str << data[*position]; escaped = false; break;
     }
     (*position)++;
  }
  (*position)++; //ignore " again
  
  return str.str();
}

JsonObject* JsonParser::ParseJsonObject(const char* data, long size, long* position)
{
  SkipEmpty(data, size, position);
  if ( data[*position] != '{' ) return NULL;
  (*position)++; //skip '{'
  bool finished = false;
  std::string error = "";
  JsonObject* jsonObject = new JsonObject();
  
  while (!finished && error.length() == 0 && *position < size) {
    SkipEmpty(data, size, position);
    if ( data[*position] == '}' || data[*position] == 0 ) {
       finished = true;
       (*position)++; // skip '}'
    } else {
      std::string name = ParseString(data, size, position);
      SkipEmpty(data, size, position);
      JsonBase* item = NULL;

      switch(data[*position]) {
        case '{': item = ParseJsonObject(data, size, position); 
                  break;
        case '"': {
                    item = (JsonBase*)new JsonBasicValue(ParseString(data, size, position));
		  } 
                  break;
        case '[': item = (JsonBase*)ParseArray(data, size, position);
                  break;
        case 't': 
        case 'f': item = (JsonBase*)ParseBool(data, size, position);
                  break;
        case 'n': if ( data[(*position)+1] == 'u' && data[(*position)+2] == 'l' && data[(*position)+3] == 'l') {
                     item = NULL; (*position) = (*position)+4;
                     break;
                  }
        default:  if((int)data[*position] >= 48 && (int)data[*position] <= 57)
                  {
                    item = (JsonBase*)ParseDouble(data, size, position);
                  } else {
                    error = "parsing failed";
     		    esyslog("restfulapi, jsonparser, error: %s, char: %c, pos: %ld", error.c_str(), data[*position], *position);
                  }
                  break;
      }
      if (error.length() == 0) {
         JsonValue* jsonValue = new JsonValue(name);
         jsonValue->Value(item);
         jsonObject->AddItem(jsonValue);
      } 
    }
  }
  if ( error.length() > 0 ) {
     delete jsonObject;
     jsonObject = NULL;
  }
  return jsonObject;
}

JsonBasicValue* JsonParser::ParseBool(const char* data, long size, long* position)
{
  //dont check size because at least t/f are available which is enough until the next loop in ParseObject or ParseArray will check the length again :-)
  if ( data[*position] == 't' ) {
     (*position) += 4;
     return new JsonBasicValue(true);
  } else {
     (*position) += 5;
     return new JsonBasicValue(false);
  }
}

JsonBasicValue* JsonParser::ParseDouble(const char* data, long size, long* position)
{
  std::ostringstream str;
  while(((data[(*position)] >= 48 && data[(*position)] <= 57) || data[(*position)] == '.') && *position < size) {
    str << data[(*position)];
    (*position)++;
  }
  double nr = atof(str.str().c_str());
  return new JsonBasicValue(nr);
}

JsonArray* JsonParser::ParseArray(const char* data, long size, long* position)
{
  SkipEmpty(data, size, position); 
  if ( data[*position] != '[' ) return NULL;
  (*position)++; //skip '[';
  SkipEmpty(data, size, position);
  JsonArray* jsonArray = new JsonArray();
  bool finished = false;
  std::string error = "";

  while (!finished && *position < size && error == "") {
    SkipEmpty(data, size, position);
    JsonBase* item = NULL;
    if (data[*position] != ']' && data[*position] != '\0') {
       switch(data[*position]) {
         case '"': item = (JsonBase*)new JsonBasicValue(ParseString(data, size, position));
                   break;
         case '{': item = (JsonBase*)ParseJsonObject(data, size, position);
                   break;
         case 'f': 
         case 't': item = (JsonBase*)ParseBool(data, size, position);
                   break;
         case 'n': if ( data[(*position)+1] == 'u' && data[(*position)+2] == 'l' && data[(*position)+3] == 'l') {
                      item = NULL; (*position) = (*position)+4;
                      break;
                   }
         default:  {
                      if (data[*position] >= 48 && data[*position] <= 57) {
                         item = (JsonBase*)ParseDouble(data, size, position);      
                      } else {
                         error = "Parsing array failed!";
                         esyslog("restfulapi, jsonparser, error: %s, char: %c, pos: %ld", error.c_str(), data[*position], *position);
                      }
                   }
                   break;
       }
       if ( error.length() == 0 && item != NULL ) {
          jsonArray->AddItem(item);
       } else if (item != NULL) {
          delete item;
       }
    } else {
       finished = true;
    }
  }

  (*position)++; //skip ']'
  return jsonArray;
}
