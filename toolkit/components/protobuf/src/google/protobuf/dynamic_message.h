




































#ifndef GOOGLE_PROTOBUF_DYNAMIC_MESSAGE_H__
#define GOOGLE_PROTOBUF_DYNAMIC_MESSAGE_H__

#include <memory>

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {


class Descriptor;        
class DescriptorPool;    


















class LIBPROTOBUF_EXPORT DynamicMessageFactory : public MessageFactory {
 public:
  
  
  DynamicMessageFactory();

  
  
  
  
  
  
  
  DynamicMessageFactory(const DescriptorPool* pool);

  ~DynamicMessageFactory();

  
  
  
  
  
  
  void SetDelegateToGeneratedFactory(bool enable) {
    delegate_to_generated_factory_ = enable;
  }

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  const Message* GetPrototype(const Descriptor* type);

 private:
  const DescriptorPool* pool_;
  bool delegate_to_generated_factory_;

  
  
  
  
  
  struct PrototypeMap;
  scoped_ptr<PrototypeMap> prototypes_;
  mutable Mutex prototypes_mutex_;

  friend class DynamicMessage;
  const Message* GetPrototypeNoLock(const Descriptor* type);

  
  
  static void ConstructDefaultOneofInstance(const Descriptor* type,
                                            const int offsets[],
                                            void* default_oneof_instance);
  
  static void DeleteDefaultOneofInstance(const Descriptor* type,
                                         const int offsets[],
                                         void* default_oneof_instance);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DynamicMessageFactory);
};

}  

}  
#endif  
