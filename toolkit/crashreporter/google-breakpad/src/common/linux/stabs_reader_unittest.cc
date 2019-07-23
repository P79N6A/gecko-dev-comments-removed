
































#include <a.out.h>
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stab.h>

#include "breakpad_googletest_includes.h"
#include "common/linux/stabs_reader.h"

using std::istream;
using std::istringstream;
using std::map;
using std::ostream;
using std::ostringstream;
using std::string;

using ::testing::_;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::StrEq;

using google_breakpad::StabsHandler;
using google_breakpad::StabsReader;

namespace {









































class MockStabsHandler {
 public:
  MockStabsHandler() { }
  virtual ~MockStabsHandler() { }
  
  
  
  
  virtual bool Entry(enum __stab_debug_code type, char other, short desc,
                     unsigned long value, const string &name) { return true; }
  
  
  
  virtual bool CUBoundary(const string &filename) { return true; }

  
  
  
  virtual bool Error(const char *format, ...) = 0;
};


class MockStabsParser {
 public:
  
  
  MockStabsParser(const string &filename, istream *stream,
                  MockStabsHandler *handler);
  
  
  
  
  bool Process();
 private:
  
  
  typedef map<string, unsigned char> StabTypeNameTable;

  
  void InitializeTypeNames();

  
  
  
  
  bool ParseLine(const string &line);

  const string &filename_;
  istream *stream_;
  MockStabsHandler *handler_;
  int line_number_;
  StabTypeNameTable type_names_;
};

MockStabsParser::MockStabsParser(const string &filename, istream *stream,
                                 MockStabsHandler *handler):
    filename_(filename), stream_(stream), handler_(handler),
    line_number_(0) {
  InitializeTypeNames();
}

bool MockStabsParser::Process() {
  
  
  for(;;) {
    string line;
    getline(*stream_, line, '\n');
    if (line.empty() && stream_->eof())
      break;
    line_number_++;
    if (! ParseLine(line))
      return false;
  }
  return true;
}

void MockStabsParser::InitializeTypeNames() {
  
  
  
  
  
  
  
  
#  define __define_stab(name, code, str) type_names_[string(str)] = code;
#  include <bits/stab.def>
#  undef __define_stab
}

bool MockStabsParser::ParseLine(const string &line) {
  istringstream linestream(line);
  
  linestream.unsetf(istringstream::basefield);
  
  string typeName;
  linestream >> typeName;
  if (typeName == "cu-boundary") {
    if (linestream.peek() == ' ')
      linestream.get();
    string filename;
    getline(linestream, filename, '\n');
    return handler_->CUBoundary(filename);
  } else {
    StabTypeNameTable::const_iterator typeIt = type_names_.find(typeName);
    if (typeIt == type_names_.end())
      return handler_->Error("%s:%d: unrecognized stab type: %s\n",
                             filename_.c_str(), line_number_, typeName.c_str());
    
    
    int otherInt, descInt;
    unsigned long value;
    linestream >> otherInt >> descInt >> value;
    if (linestream.fail())
      return handler_->Error("%s:%d: malformed mock stabs input line\n",
                             filename_.c_str(), line_number_);
    if (linestream.peek() == ' ')
      linestream.get();
    string name;
    getline(linestream, name, '\n');
    return handler_->Entry(static_cast<__stab_debug_code>(typeIt->second),
                           otherInt, descInt, value, name);
  }
}






class StabSection {
 public:
  StabSection(): used_(0), size_(1) {
    entries_ = (struct nlist *) malloc(sizeof(*entries_) * size_);
  }
  ~StabSection() { free(entries_); }

  
  
  
  
  struct nlist *Append();
  
  
  void SetHeader(short count, unsigned long string_size);
  
  
  void GetSection(string *section);
  
  
  void Clear();

 private:
  
  struct nlist *entries_;
  
  
  size_t used_, size_;
};

struct nlist *StabSection::Append() {
  if (used_ == size_) {
    size_ *= 2;
    entries_ = (struct nlist *) realloc(entries_, sizeof(*entries_) * size_);
  }
  assert(used_ < size_);
  return &entries_[used_++];
}

void StabSection::SetHeader(short count, unsigned long string_size) {
  assert(used_ >= 1);
  entries_[0].n_desc = count;
  entries_[0].n_value = string_size;
}

void StabSection::GetSection(string *section) {
  section->assign(reinterpret_cast<char *>(entries_),
                  sizeof(*entries_) * used_);
}

void StabSection::Clear() {
  used_ = 0;
  size_ = 1;
  entries_ = (struct nlist *) realloc(entries_, sizeof(*entries_) * size_);
}













class StabstrSection {
 public:
  StabstrSection(): next_byte_(1) { string_indices_[""] = 0; }
  
  size_t Insert(const string &str);
  
  
  void GetSection(string *section);
  
  
  void Clear();
 private:
  
  typedef map<string, size_t> StringToIndex;
  typedef map<size_t, const string *> IndexToString;

  
  StringToIndex string_indices_;

  
  
  size_t next_byte_;
};

size_t StabstrSection::Insert(const string &str) {
  StringToIndex::iterator it = string_indices_.find(str);
  size_t index;
  if (it != string_indices_.end()) {
    index = it->second;
  } else {
    
    string_indices_[str] = next_byte_;
    index = next_byte_;
    next_byte_ += str.size() + 1;
  }
  return index;
}

void StabstrSection::GetSection(string *section) {
  
  IndexToString byIndex;
  for (StringToIndex::const_iterator it = string_indices_.begin();
       it != string_indices_.end(); it++)
    byIndex[it->second] = &it->first;
  
  section->clear();
  for (IndexToString::const_iterator it = byIndex.begin();
       it != byIndex.end(); it++) {
    
    assert(it->first == section->size());
    *section += *(it->second);
    *section += '\0';
  }
}

void StabstrSection::Clear() {
  string_indices_.clear();
  string_indices_[""] = 0;
  next_byte_ = 1;
}



class StabsSectionsBuilder: public MockStabsHandler {
 public:
  
  
  
  
  StabsSectionsBuilder(const string &filename)
      : filename_(filename), error_count_(0), has_header_(false),
        entry_count_(0) { }

  
  bool Entry(enum __stab_debug_code type, char other, short desc,
             unsigned long value, const string &name);
  bool CUBoundary(const string &filename);
  bool Error(const char *format, ...);

  
  
  void GetStab(string *section);
  void GetStabstr(string *section);

 private:
  
  
  
  
  void FinishCU();

  const string &filename_;        
  int error_count_;               

  
  
  bool has_header_;               
  int entry_count_;               
  StabSection stab_;              
  StabstrSection stabstr_;        

  
  string finished_cu_stab_, finished_cu_stabstr_;
};

bool StabsSectionsBuilder::Entry(enum __stab_debug_code type, char other,
                                 short desc, unsigned long value,
                                 const string &name) {
  struct nlist *entry = stab_.Append();
  entry->n_type = type;
  entry->n_other = other;
  entry->n_desc = desc;
  entry->n_value = value;
  entry->n_un.n_strx = stabstr_.Insert(name);
  entry_count_++;
  return true;
}

bool StabsSectionsBuilder::CUBoundary(const string &filename) {
  FinishCU();
  
  assert(!has_header_);
  assert(entry_count_ == 0);
  struct nlist *entry = stab_.Append();
  entry->n_type = N_UNDF;
  entry->n_other = 0;
  entry->n_desc = 0;          
  entry->n_value = 0;         
  entry->n_un.n_strx = stabstr_.Insert(filename);
  has_header_ = true;
  
  
  return true;
}

void StabsSectionsBuilder::FinishCU() {
  if (entry_count_ > 0) {
    
    string stabstr;
    stabstr_.GetSection(&stabstr);
    finished_cu_stabstr_ += stabstr;

    
    if (has_header_)
      stab_.SetHeader(entry_count_, stabstr.size());
    string stab;
    stab_.GetSection(&stab);
    finished_cu_stab_ += stab;
  }

  stab_.Clear();
  stabstr_.Clear();
  has_header_ = false;
  entry_count_ = 0;
}

bool StabsSectionsBuilder::Error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  error_count_++;
  if (error_count_ >= 20) {
    fprintf(stderr,
            "%s: lots of errors; is this really a mock stabs file?\n",
            filename_.c_str());
    return false;
  }
  return true;
}

void StabsSectionsBuilder::GetStab(string *section) {
  FinishCU();
  *section = finished_cu_stab_;
}

void StabsSectionsBuilder::GetStabstr(string *section) {
  FinishCU();
  *section = finished_cu_stabstr_;
}

class MockStabsReaderHandler: public StabsHandler {
 public:
  MOCK_METHOD3(StartCompilationUnit,
               bool(const char *, uint64_t, const char *));
  MOCK_METHOD1(EndCompilationUnit, bool(uint64_t));
  MOCK_METHOD2(StartFunction, bool(const std::string &, uint64_t));
  MOCK_METHOD1(EndFunction, bool(uint64_t));
  MOCK_METHOD3(Line, bool(uint64_t, const char *, int));
  void Warning(const char *format, ...) { MockWarning(format); }
  MOCK_METHOD1(MockWarning, void(const char *));
};






static bool ApplyHandlerToMockStabsData(StabsHandler *handler,
                                        const string &input_file) {
  string full_input_file
      = string(getenv("srcdir") ? getenv("srcdir") : ".") + "/" + input_file;

  
  std::ifstream stream(full_input_file.c_str());
  if (stream.fail()) {
    fprintf(stderr, "error opening mock stabs input file %s: %s\n",
            full_input_file.c_str(), strerror(errno));
    return false;
  }

  
  
  StabsSectionsBuilder builder(full_input_file);
  MockStabsParser mock_parser(full_input_file, &stream, &builder);
  if (!mock_parser.Process())
    return false;
  string stab, stabstr;
  builder.GetStab(&stab);
  builder.GetStabstr(&stabstr);

  
  StabsReader reader(
      reinterpret_cast<const uint8_t *>(stab.data()),    stab.size(),
      reinterpret_cast<const uint8_t *>(stabstr.data()), stabstr.size(),
      handler);
  return reader.Process();
}

TEST(StabsReader, MockStabsInput) {
  MockStabsReaderHandler mock_handler;

  {
    InSequence s;

    EXPECT_CALL(mock_handler, StartCompilationUnit(StrEq("file1.c"), 
                                                   0x42, StrEq("builddir1/")))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(StrEq("fun1"), 0x62))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, Line(0xe4, StrEq("file1.c"), 91))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, Line(0x164, StrEq("header.h"), 111))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0x112))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(StrEq("fun2"), 0x112))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, Line(0x234, StrEq("header.h"), 131))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, Line(0x254, StrEq("file1.c"), 151))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0x152))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x152))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartCompilationUnit(StrEq("file3.c"), 
                                                   0x182, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x192))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData(
                  &mock_handler, 
                  "common/linux/testdata/stabs_reader_unittest.input1"));
}

TEST(StabsReader, AbruptCU) {
  MockStabsReaderHandler mock_handler;

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file2-1.c"), 0x12, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(NULL))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData(
                  &mock_handler, 
                  "common/linux/testdata/stabs_reader_unittest.input2"));
}

TEST(StabsReader, AbruptFunction) {
  MockStabsReaderHandler mock_handler;

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file3-1.c"), 0x12, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(StrEq("fun3_1"), 0x22))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(NULL))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData(
                  &mock_handler, 
                  "common/linux/testdata/stabs_reader_unittest.input3"));
}

TEST(StabsReader, NoCU) {
  MockStabsReaderHandler mock_handler;

  EXPECT_CALL(mock_handler, StartCompilationUnit(_, _, _))
      .Times(0);
  EXPECT_CALL(mock_handler, StartFunction(_, _))
      .Times(0);

  ASSERT_TRUE(ApplyHandlerToMockStabsData(
                  &mock_handler, 
                  "common/linux/testdata/stabs_reader_unittest.input4"));
  
}

TEST(StabsReader, NoCUEnd) {
  MockStabsReaderHandler mock_handler;

  {
    InSequence s;

    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file5-1.c"), 0x12, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("file5-2.c"), 0x22, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(NULL))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData(
                  &mock_handler, 
                  "common/linux/testdata/stabs_reader_unittest.input5"));
  
}

TEST(StabsReader, MultipleCUs) {
  MockStabsReaderHandler mock_handler;

  {
    InSequence s;
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("antimony"), 0x12, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(Eq("arsenic"), 0x22))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0x32))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x32))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler,
                StartCompilationUnit(StrEq("aluminum"), 0x42, NULL))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, StartFunction(Eq("selenium"), 0x52))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndFunction(0x62))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_handler, EndCompilationUnit(0x62))
        .WillOnce(Return(true));
  }

  ASSERT_TRUE(ApplyHandlerToMockStabsData(
                  &mock_handler,
                  "common/linux/testdata/stabs_reader_unittest.input6"));
}



} 
