









#include "webrtc/system_wrappers/interface/data_log.h"

#include <string>

namespace webrtc {

int DataLog::CreateLog() {
  return 0;
}

void DataLog::ReturnLog() {
}

std::string DataLog::Combine(const std::string& table_name, int table_id) {
  return std::string();
}

int DataLog::AddTable(const std::string& ) {
  return 0;
}

int DataLog::AddColumn(const std::string& ,
                       const std::string& ,
                       int ) {
  return 0;
}

int DataLog::NextRow(const std::string& ) {
  return 0;
}

DataLogImpl::DataLogImpl() {
}

DataLogImpl::~DataLogImpl() {
}

DataLogImpl* DataLogImpl::StaticInstance() {
  return NULL;
}

void DataLogImpl::ReturnLog() {
}

int DataLogImpl::AddTable(const std::string& ) {
  return 0;
}

int DataLogImpl::AddColumn(const std::string& ,
                           const std::string& ,
                           int ) {
  return 0;
}

int DataLogImpl::InsertCell(const std::string& ,
                            const std::string& ,
                            const Container* ) {
  return 0;
}

int DataLogImpl::NextRow(const std::string& ) {
  return 0;
}

void DataLogImpl::Flush() {
}

bool DataLogImpl::Run(void* ) {
  return true;
}

void DataLogImpl::Process() {
}

void DataLogImpl::StopThread() {
}

}  
