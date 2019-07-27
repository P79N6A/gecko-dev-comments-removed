









#ifndef WEBRTC_BASE_JSON_H_
#define WEBRTC_BASE_JSON_H_

#include <string>
#include <vector>

#if !defined(WEBRTC_EXTERNAL_JSON)
#include "json/json.h"
#else
#include "third_party/jsoncpp/json.h"
#endif








bool GetIntFromJson(const Json::Value& in, int* out);
bool GetUIntFromJson(const Json::Value& in, unsigned int* out);
bool GetStringFromJson(const Json::Value& in, std::string* out);
bool GetBoolFromJson(const Json::Value& in, bool* out);
bool GetDoubleFromJson(const Json::Value& in, double* out);


bool GetValueFromJsonArray(const Json::Value& in, size_t n,
                           Json::Value* out);
bool GetIntFromJsonArray(const Json::Value& in, size_t n,
                         int* out);
bool GetUIntFromJsonArray(const Json::Value& in, size_t n,
                          unsigned int* out);
bool GetStringFromJsonArray(const Json::Value& in, size_t n,
                            std::string* out);
bool GetBoolFromJsonArray(const Json::Value& in, size_t n,
                          bool* out);
bool GetDoubleFromJsonArray(const Json::Value& in, size_t n,
                            double* out);


bool JsonArrayToValueVector(const Json::Value& in,
                            std::vector<Json::Value>* out);
bool JsonArrayToIntVector(const Json::Value& in,
                          std::vector<int>* out);
bool JsonArrayToUIntVector(const Json::Value& in,
                           std::vector<unsigned int>* out);
bool JsonArrayToStringVector(const Json::Value& in,
                             std::vector<std::string>* out);
bool JsonArrayToBoolVector(const Json::Value& in,
                           std::vector<bool>* out);
bool JsonArrayToDoubleVector(const Json::Value& in,
                             std::vector<double>* out);


Json::Value ValueVectorToJsonArray(const std::vector<Json::Value>& in);
Json::Value IntVectorToJsonArray(const std::vector<int>& in);
Json::Value UIntVectorToJsonArray(const std::vector<unsigned int>& in);
Json::Value StringVectorToJsonArray(const std::vector<std::string>& in);
Json::Value BoolVectorToJsonArray(const std::vector<bool>& in);
Json::Value DoubleVectorToJsonArray(const std::vector<double>& in);


bool GetValueFromJsonObject(const Json::Value& in, const std::string& k,
                            Json::Value* out);
bool GetIntFromJsonObject(const Json::Value& in, const std::string& k,
                          int* out);
bool GetUIntFromJsonObject(const Json::Value& in, const std::string& k,
                           unsigned int* out);
bool GetStringFromJsonObject(const Json::Value& in, const std::string& k,
                             std::string* out);
bool GetBoolFromJsonObject(const Json::Value& in, const std::string& k,
                           bool* out);
bool GetDoubleFromJsonObject(const Json::Value& in, const std::string& k,
                             double* out);


std::string JsonValueToString(const Json::Value& json);

#endif  
