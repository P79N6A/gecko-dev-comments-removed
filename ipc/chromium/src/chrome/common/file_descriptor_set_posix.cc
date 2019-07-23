



#include "chrome/common/file_descriptor_set_posix.h"

#include "base/eintr_wrapper.h"
#include "base/logging.h"

FileDescriptorSet::FileDescriptorSet()
    : consumed_descriptor_highwater_(0) {
}

FileDescriptorSet::~FileDescriptorSet() {
  if (consumed_descriptor_highwater_ == descriptors_.size())
    return;

  LOG(WARNING) << "FileDescriptorSet destroyed with unconsumed descriptors";
  
  
  
  
  
  
  
  
  for (unsigned i = consumed_descriptor_highwater_;
       i < descriptors_.size(); ++i) {
    if (descriptors_[i].auto_close)
      HANDLE_EINTR(close(descriptors_[i].fd));
  }
}

bool FileDescriptorSet::Add(int fd) {
  if (descriptors_.size() == MAX_DESCRIPTORS_PER_MESSAGE)
    return false;

  struct base::FileDescriptor sd;
  sd.fd = fd;
  sd.auto_close = false;
  descriptors_.push_back(sd);
  return true;
}

bool FileDescriptorSet::AddAndAutoClose(int fd) {
  if (descriptors_.size() == MAX_DESCRIPTORS_PER_MESSAGE)
    return false;

  struct base::FileDescriptor sd;
  sd.fd = fd;
  sd.auto_close = true;
  descriptors_.push_back(sd);
  DCHECK(descriptors_.size() <= MAX_DESCRIPTORS_PER_MESSAGE);
  return true;
}

int FileDescriptorSet::GetDescriptorAt(unsigned index) const {
  if (index >= descriptors_.size())
    return -1;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (index == 0 && consumed_descriptor_highwater_ == descriptors_.size())
    consumed_descriptor_highwater_ = 0;

  if (index != consumed_descriptor_highwater_)
    return -1;

  consumed_descriptor_highwater_ = index + 1;
  return descriptors_[index].fd;
}

void FileDescriptorSet::GetDescriptors(int* buffer) const {
  for (std::vector<base::FileDescriptor>::const_iterator
       i = descriptors_.begin(); i != descriptors_.end(); ++i) {
    *(buffer++) = i->fd;
  }
}

void FileDescriptorSet::CommitAll() {
  for (std::vector<base::FileDescriptor>::iterator
       i = descriptors_.begin(); i != descriptors_.end(); ++i) {
    if (i->auto_close)
      HANDLE_EINTR(close(i->fd));
  }
  descriptors_.clear();
  consumed_descriptor_highwater_ = 0;
}

void FileDescriptorSet::SetDescriptors(const int* buffer, unsigned count) {
  DCHECK_LE(count, MAX_DESCRIPTORS_PER_MESSAGE);
  DCHECK_EQ(descriptors_.size(), 0u);
  DCHECK_EQ(consumed_descriptor_highwater_, 0u);

  descriptors_.reserve(count);
  for (unsigned i = 0; i < count; ++i) {
    struct base::FileDescriptor sd;
    sd.fd = buffer[i];
    sd.auto_close = true;
    descriptors_.push_back(sd);
  }
}
