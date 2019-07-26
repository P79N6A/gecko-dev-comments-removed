









#include "webrtc/system_wrappers/interface/data_log.h"

#include <assert.h>
#include <algorithm>
#include <list>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/file_wrapper.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"

namespace webrtc {

DataLogImpl::CritSectScopedPtr DataLogImpl::crit_sect_(
  CriticalSectionWrapper::CreateCriticalSection());

DataLogImpl* DataLogImpl::instance_ = NULL;



class Row {
 public:
  Row();
  ~Row();

  
  
  
  int InsertCell(const std::string& column_name,
                 const Container* value_container);

  
  
  
  void ToString(const std::string& column_name, std::string* value_string);

 private:
  
  typedef std::map<std::string, const Container*> CellMap;

  CellMap                   cells_;
  CriticalSectionWrapper*   cells_lock_;
};





class LogTable {
 public:
  LogTable();
  ~LogTable();

  
  
  
  int AddColumn(const std::string& column_name, int multi_value_length);

  
  
  
  void NextRow();

  
  
  
  int InsertCell(const std::string& column_name,
                 const Container* value_container);

  
  
  int CreateLogFile(const std::string& file_name);

  
  
  
  
  void Flush();

 private:
  
  typedef std::map<std::string, int> ColumnMap;
  typedef std::list<Row*> RowList;

  ColumnMap               columns_;
  RowList                 rows_[2];
  RowList*                rows_history_;
  RowList*                rows_flush_;
  Row*                    current_row_;
  FileWrapper*            file_;
  bool                    write_header_;
  CriticalSectionWrapper* table_lock_;
};

Row::Row()
  : cells_(),
    cells_lock_(CriticalSectionWrapper::CreateCriticalSection()) {
}

Row::~Row() {
  for (CellMap::iterator it = cells_.begin(); it != cells_.end();) {
    delete it->second;
    
    cells_.erase(it++);
  }
  delete cells_lock_;
}

int Row::InsertCell(const std::string& column_name,
                    const Container* value_container) {
  CriticalSectionScoped synchronize(cells_lock_);
  assert(cells_.count(column_name) == 0);
  if (cells_.count(column_name) > 0)
    return -1;
  cells_[column_name] = value_container;
  return 0;
}

void Row::ToString(const std::string& column_name,
                   std::string* value_string) {
  CriticalSectionScoped synchronize(cells_lock_);
  const Container* container = cells_[column_name];
  if (container == NULL) {
    *value_string = "NaN,";
    return;
  }
  container->ToString(value_string);
}

LogTable::LogTable()
  : columns_(),
    rows_(),
    rows_history_(&rows_[0]),
    rows_flush_(&rows_[1]),
    current_row_(new Row),
    file_(FileWrapper::Create()),
    write_header_(true),
    table_lock_(CriticalSectionWrapper::CreateCriticalSection()) {
}

LogTable::~LogTable() {
  for (RowList::iterator row_it = rows_history_->begin();
       row_it != rows_history_->end();) {
    delete *row_it;
    row_it = rows_history_->erase(row_it);
  }
  for (ColumnMap::iterator col_it = columns_.begin();
       col_it != columns_.end();) {
    
    columns_.erase(col_it++);
  }
  if (file_ != NULL) {
    file_->Flush();
    file_->CloseFile();
    delete file_;
  }
  delete current_row_;
  delete table_lock_;
}

int LogTable::AddColumn(const std::string& column_name,
                        int multi_value_length) {
  assert(multi_value_length > 0);
  if (!write_header_) {
    
    
    assert(false);
    return -1;
  } else {
    CriticalSectionScoped synchronize(table_lock_);
    if (write_header_)
      columns_[column_name] = multi_value_length;
    else
      return -1;
  }
  return 0;
}

void LogTable::NextRow() {
  CriticalSectionScoped sync_rows(table_lock_);
  rows_history_->push_back(current_row_);
  current_row_ = new Row;
}

int LogTable::InsertCell(const std::string& column_name,
                         const Container* value_container) {
  CriticalSectionScoped synchronize(table_lock_);
  assert(columns_.count(column_name) > 0);
  if (columns_.count(column_name) == 0)
    return -1;
  return current_row_->InsertCell(column_name, value_container);
}

int LogTable::CreateLogFile(const std::string& file_name) {
  if (file_name.length() == 0)
    return -1;
  if (file_->Open())
    return -1;
  file_->OpenFile(file_name.c_str(),
                  false,  
                  false,  
                          
                  true);  
  if (file_ == NULL)
    return -1;
  return 0;
}

void LogTable::Flush() {
  ColumnMap::iterator column_it;
  bool commit_header = false;
  if (write_header_) {
    CriticalSectionScoped synchronize(table_lock_);
    if (write_header_) {
      commit_header = true;
      write_header_ = false;
    }
  }
  if (commit_header) {
    for (column_it = columns_.begin();
         column_it != columns_.end(); ++column_it) {
      if (column_it->second > 1) {
        file_->WriteText("%s[%u],", column_it->first.c_str(),
                         column_it->second);
        for (int i = 1; i < column_it->second; ++i)
          file_->WriteText(",");
      } else {
        file_->WriteText("%s,", column_it->first.c_str());
      }
    }
    if (columns_.size() > 0)
      file_->WriteText("\n");
  }

  
  
  
  
  
  {
    CriticalSectionScoped synchronize(table_lock_);
    RowList* tmp = rows_flush_;
    rows_flush_ = rows_history_;
    rows_history_ = tmp;
    rows_history_->clear();
  }

  
  for (RowList::iterator row_it = rows_flush_->begin();
       row_it != rows_flush_->end();) {
    for (column_it = columns_.begin();
         column_it != columns_.end(); ++column_it) {
      std::string row_string;
      (*row_it)->ToString(column_it->first, &row_string);
      file_->WriteText("%s", row_string.c_str());
    }
    if (columns_.size() > 0)
      file_->WriteText("\n");
    delete *row_it;
    row_it = rows_flush_->erase(row_it);
  }
}

int DataLog::CreateLog() {
  return DataLogImpl::CreateLog();
}

void DataLog::ReturnLog() {
  return DataLogImpl::ReturnLog();
}

std::string DataLog::Combine(const std::string& table_name, int table_id) {
  std::stringstream ss;
  std::string combined_id = table_name;
  std::string number_suffix;
  ss << "_" << table_id;
  ss >> number_suffix;
  combined_id += number_suffix;
  std::transform(combined_id.begin(), combined_id.end(), combined_id.begin(),
                 ::tolower);
  return combined_id;
}

int DataLog::AddTable(const std::string& table_name) {
  DataLogImpl* data_log = DataLogImpl::StaticInstance();
  if (data_log == NULL)
    return -1;
  return data_log->AddTable(table_name);
}

int DataLog::AddColumn(const std::string& table_name,
                       const std::string& column_name,
                       int multi_value_length) {
  DataLogImpl* data_log = DataLogImpl::StaticInstance();
  if (data_log == NULL)
    return -1;
  return data_log->DataLogImpl::StaticInstance()->AddColumn(table_name,
                                                            column_name,
                                                            multi_value_length);
}

int DataLog::NextRow(const std::string& table_name) {
  DataLogImpl* data_log = DataLogImpl::StaticInstance();
  if (data_log == NULL)
    return -1;
  return data_log->DataLogImpl::StaticInstance()->NextRow(table_name);
}

DataLogImpl::DataLogImpl()
  : counter_(1),
    tables_(),
    flush_event_(EventWrapper::Create()),
    file_writer_thread_(NULL),
    tables_lock_(RWLockWrapper::CreateRWLock()) {
}

DataLogImpl::~DataLogImpl() {
  StopThread();
  Flush();  
  delete file_writer_thread_;
  delete flush_event_;
  for (TableMap::iterator it = tables_.begin(); it != tables_.end();) {
    delete static_cast<LogTable*>(it->second);
    
    tables_.erase(it++);
  }
  delete tables_lock_;
}

int DataLogImpl::CreateLog() {
  CriticalSectionScoped synchronize(crit_sect_.get());
  if (instance_ == NULL) {
    instance_ = new DataLogImpl();
    return instance_->Init();
  } else {
    ++instance_->counter_;
  }
  return 0;
}

int DataLogImpl::Init() {
  file_writer_thread_ = ThreadWrapper::CreateThread(
                          DataLogImpl::Run,
                          instance_,
                          kHighestPriority,
                          "DataLog");
  if (file_writer_thread_ == NULL)
    return -1;
  unsigned int thread_id = 0;
  bool success = file_writer_thread_->Start(thread_id);
  if (!success)
    return -1;
  return 0;
}

DataLogImpl* DataLogImpl::StaticInstance() {
  return instance_;
}

void DataLogImpl::ReturnLog() {
  CriticalSectionScoped synchronize(crit_sect_.get());
  if (instance_ && instance_->counter_ > 1) {
    --instance_->counter_;
    return;
  }
  delete instance_;
  instance_ = NULL;
}

int DataLogImpl::AddTable(const std::string& table_name) {
  WriteLockScoped synchronize(*tables_lock_);
  
  if (tables_.count(table_name) > 0)
    return -1;
  tables_[table_name] = new LogTable();
  if (tables_[table_name]->CreateLogFile(table_name + ".txt") == -1)
    return -1;
  return 0;
}

int DataLogImpl::AddColumn(const std::string& table_name,
                           const std::string& column_name,
                           int multi_value_length) {
  ReadLockScoped synchronize(*tables_lock_);
  if (tables_.count(table_name) == 0)
    return -1;
  return tables_[table_name]->AddColumn(column_name, multi_value_length);
}

int DataLogImpl::InsertCell(const std::string& table_name,
                            const std::string& column_name,
                            const Container* value_container) {
  ReadLockScoped synchronize(*tables_lock_);
  assert(tables_.count(table_name) > 0);
  if (tables_.count(table_name) == 0)
    return -1;
  return tables_[table_name]->InsertCell(column_name, value_container);
}

int DataLogImpl::NextRow(const std::string& table_name) {
  ReadLockScoped synchronize(*tables_lock_);
  if (tables_.count(table_name) == 0)
    return -1;
  tables_[table_name]->NextRow();
  if (file_writer_thread_ == NULL) {
    
    tables_[table_name]->Flush();
  } else {
    
    flush_event_->Set();
  }
  return 0;
}

void DataLogImpl::Flush() {
  ReadLockScoped synchronize(*tables_lock_);
  for (TableMap::iterator it = tables_.begin(); it != tables_.end(); ++it) {
    it->second->Flush();
  }
}

bool DataLogImpl::Run(void* obj) {
  static_cast<DataLogImpl*>(obj)->Process();
  return true;
}

void DataLogImpl::Process() {
  
  flush_event_->Wait(WEBRTC_EVENT_INFINITE);
  Flush();
}

void DataLogImpl::StopThread() {
  if (file_writer_thread_ != NULL) {
    file_writer_thread_->SetNotAlive();
    flush_event_->Set();
    
    
    while (!file_writer_thread_->Stop()) continue;
  }
}

}  
