



#ifndef CHROME_COMMON_FILE_DESCRIPTOR_SET_POSIX_H_
#define CHROME_COMMON_FILE_DESCRIPTOR_SET_POSIX_H_

#include <vector>

#include "base/basictypes.h"
#include "base/file_descriptor_posix.h"
#include "base/ref_counted.h"






class FileDescriptorSet : public base::RefCountedThreadSafe<FileDescriptorSet> {
 public:
  FileDescriptorSet();
  ~FileDescriptorSet();

  
  
  
  
  
  
  
  
  enum {
    MAX_DESCRIPTORS_PER_MESSAGE = 4,
  };

  
  

  
  bool Add(int fd);
  
  
  bool AddAndAutoClose(int fd);

  


  
  

  
  unsigned size() const { return descriptors_.size(); }
  
  bool empty() const { return descriptors_.empty(); }
  
  
  
  
  
  
  
  int GetDescriptorAt(unsigned n) const;

  


  
  

  
  
  
  void GetDescriptors(int* buffer) const;
  
  
  
  void CommitAll();

  


  
  

  
  
  
  void SetDescriptors(const int* buffer, unsigned count);

  

 private:
  
  
  
  
  std::vector<base::FileDescriptor> descriptors_;

  
  
  
  
  mutable unsigned consumed_descriptor_highwater_;

  DISALLOW_COPY_AND_ASSIGN(FileDescriptorSet);
};

#endif  
