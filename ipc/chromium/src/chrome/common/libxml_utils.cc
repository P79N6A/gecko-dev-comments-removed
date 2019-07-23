



#include "chrome/common/libxml_utils.h"

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/string_util.h"

#include "libxml/xmlreader.h"

std::string XmlStringToStdString(const xmlChar* xmlstring) {
  
  if (xmlstring)
    return std::string(reinterpret_cast<const char*>(xmlstring));
  else
    return "";
}

XmlReader::XmlReader()
    : reader_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(
          error_func_(this, &XmlReader::GenericErrorCallback)) {
}

XmlReader::~XmlReader() {
  if (reader_)
    xmlFreeTextReader(reader_);
}

 void XmlReader::GenericErrorCallback(void* context,
                                                  const char* msg, ...) {
  va_list args;
  va_start(args, msg);

  XmlReader* reader = static_cast<XmlReader*>(context);
  reader->errors_.append(StringPrintf(msg, args));
}

bool XmlReader::Load(const std::string& input) {
  const int kParseOptions =
      XML_PARSE_RECOVER |  
      XML_PARSE_NONET;     
  
  
  reader_ = xmlReaderForMemory(input.data(), static_cast<int>(input.size()),
                               NULL, NULL, kParseOptions);
  return reader_ != NULL;
}

bool XmlReader::LoadFile(const std::string& file_path) {

  const int kParseOptions =
      XML_PARSE_RECOVER |  
      XML_PARSE_NONET;     
  reader_ = xmlReaderForFile(file_path.c_str(), NULL, kParseOptions);
  return reader_ != NULL;
}

bool XmlReader::NodeAttribute(const char* name, std::string* out) {
  xmlChar* value = xmlTextReaderGetAttribute(reader_, BAD_CAST name);
  if (!value)
    return false;
  *out = XmlStringToStdString(value);
  xmlFree(value);
  return true;
}

bool XmlReader::ReadElementContent(std::string* content) {
  DCHECK(NodeType() == XML_READER_TYPE_ELEMENT);
  const int start_depth = Depth();

  if (xmlTextReaderIsEmptyElement(reader_)) {
    
    
    *content = "";
    
    if (!Read())
      return false;
    return true;
  }

  
  if (!Read())
    return false;

  
  
  while (NodeType() != XML_READER_TYPE_END_ELEMENT || Depth() != start_depth) {
    *content += XmlStringToStdString(xmlTextReaderConstValue(reader_));
    if (!Read())
      return false;
  }

  
  DCHECK_EQ(NodeType(), XML_READER_TYPE_END_ELEMENT);
  if (!Read())
    return false;

  return true;
}

bool XmlReader::SkipToElement() {
  do {
    switch (NodeType()) {
    case XML_READER_TYPE_ELEMENT:
      return true;
    case XML_READER_TYPE_END_ELEMENT:
      return false;
    default:
      
      continue;
    }
  } while (Read());
  return false;
}




XmlWriter::XmlWriter() :
    writer_(NULL),
    buffer_(NULL) {}

XmlWriter::~XmlWriter() {
  if (writer_)
    xmlFreeTextWriter(writer_);
  if (buffer_)
    xmlBufferFree(buffer_);
}

void XmlWriter::StartWriting() {
  buffer_ = xmlBufferCreate();
  writer_ = xmlNewTextWriterMemory(buffer_, 0);
  xmlTextWriterSetIndent(writer_, 1);
  xmlTextWriterStartDocument(writer_, NULL, NULL, NULL);
}

void XmlWriter::StopWriting() {
  xmlTextWriterEndDocument(writer_);
  xmlFreeTextWriter(writer_);
  writer_ = NULL;
}
