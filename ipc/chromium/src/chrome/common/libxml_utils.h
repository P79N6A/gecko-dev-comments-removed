



#ifndef CHROME_COMMON_LIBXML_UTILS_H__
#define CHROME_COMMON_LIBXML_UTILS_H__

#include <string>

#include "libxml/xmlreader.h"
#include "libxml/xmlwriter.h"



std::string XmlStringToStdString(const xmlChar* xmlstring);




class ScopedXmlErrorFunc {
 public:
  ScopedXmlErrorFunc(void* context, xmlGenericErrorFunc func) {
    old_error_func_ = xmlGenericError;
    old_error_context_ = xmlGenericErrorContext;
    xmlSetGenericErrorFunc(context, func);
  }
  ~ScopedXmlErrorFunc() {
    xmlSetGenericErrorFunc(old_error_context_, old_error_func_);
  }

 private:
  xmlGenericErrorFunc old_error_func_;
  void* old_error_context_;
};



class XmlReader {
 public:
  XmlReader();
  ~XmlReader();

  
  
  
  bool Load(const std::string& input);

  
  bool LoadFile(const std::string& file_path);

  

  
  bool Read() { return xmlTextReaderRead(reader_) == 1; }

  
  
  bool Next() { return xmlTextReaderNext(reader_) == 1; }

  
  int Depth() { return xmlTextReaderDepth(reader_); }

  
  
  std::string NodeName() {
    return XmlStringToStdString(xmlTextReaderConstLocalName(reader_));
  }

  
  
  
  
  bool NodeAttribute(const char* name, std::string* value);

  

  
  
  
  
  
  
  bool ReadElementContent(std::string* content);

  
  
  
  bool SkipToElement();

  
  
  const std::string& errors() const { return errors_; }

 private:
  
  static void GenericErrorCallback(void* context, const char* msg, ...);

  
  int NodeType() { return xmlTextReaderNodeType(reader_); }

  
  xmlTextReaderPtr reader_;

  
  
  ScopedXmlErrorFunc error_func_;
  std::string errors_;
};





class XmlWriter {
 public:
  XmlWriter();
  ~XmlWriter();

  
  
  
  void StartWriting();

  
  
  void StopWriting();
  

  
  void StartIndenting() { xmlTextWriterSetIndent(writer_, 1); }

  
  void StopIndenting() { xmlTextWriterSetIndent(writer_, 0); }

  
  
  bool StartElement(const std::string& element_name) {
    return xmlTextWriterStartElement(writer_,
                                     BAD_CAST element_name.c_str()) >= 0;
  }

  
  bool EndElement() {
    return xmlTextWriterEndElement(writer_) >= 0;
  }

  
  bool AddAttribute(const std::string& attribute_name,
                    const std::string& attribute_value) {
    return xmlTextWriterWriteAttribute(writer_,
                                       BAD_CAST attribute_name.c_str(),
                                       BAD_CAST attribute_value.c_str()) >= 0;
  }

  
  
  
  bool WriteElement(const std::string& element_name,
                    const std::string& content) {
    return xmlTextWriterWriteElement(writer_,
                                     BAD_CAST element_name.c_str(),
                                     BAD_CAST content.c_str()) >= 0;
  }

  

  
  std::string GetWrittenString() {
    if (buffer_ == NULL)
      return "";
    return XmlStringToStdString(buffer_->content);
  }

 private:
  
  xmlTextWriterPtr writer_;

  
  xmlBufferPtr buffer_;
};

#endif  
