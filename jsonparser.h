#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <vdr/plugin.h>

#ifndef __JSONPARSER_H
#define __JSONPARSER_H

class JsonBase
{
  private:
   int _type;
  public:
    JsonBase(int type);
    ~JsonBase();
    bool IsObject();
    bool IsValue();
    bool IsBasicValue();
    bool IsArray();
};


class JsonValue : public JsonBase
{
  private:
    std::string _identifier;
    JsonBase* _value;
  public:
    JsonValue(std::string identifier);
    ~JsonValue();
    std::string Identifier();
    void Value(JsonBase* value);
    JsonBase* Value();
};

class JsonBasicValue : public JsonBase
{
  private:
    std::string _str;
    double _double;
    bool _bool;
    int _type;
  public:
    JsonBasicValue(std::string value);
    JsonBasicValue(double value);
    JsonBasicValue(bool value);
    ~JsonBasicValue();
    bool IsString();
    bool IsBool();
    bool IsDouble();
    std::string ValueAsString();
    double ValueAsDouble();
    bool ValueAsBool();
};

class JsonArray : public JsonBase
{
  private:
    std::vector< JsonBase* > _items;
  public:
    JsonArray();
    ~JsonArray();
    void AddItem(JsonBase* item);
    JsonBase* GetItem(int i);
    int CountItem();
};

class JsonObject : public JsonBase
{
  private:
    std::vector< JsonValue* > _items;
  public:
    JsonObject();
    ~JsonObject();
    void AddItem(JsonValue* item);
    JsonValue* GetItem(int i);
    JsonValue* GetItem(std::string str);
    JsonValue* GetItem(const char* str);
    int CountItem();
};

class JsonParser
{
  public:
    JsonParser();
    ~JsonParser();
    JsonObject* Parse(std::string str);
  private:
    char QUOTATIONCHAR;
    bool SkipEmpty(const char* data, long size, long* position);
    std::string ParseString(const char* data, long size, long* position);
    JsonObject* ParseJsonObject(const char* data, long size, long* position);
    JsonBasicValue* ParseBool(const char* data, long size, long* position);
    JsonBasicValue* ParseDouble(const char* data, long size, long* position);
    JsonArray* ParseArray(const char* data, long size, long* position);
};

#endif


