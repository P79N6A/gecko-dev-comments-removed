



#ifndef BASE_FILE_DESCRIPTOR_SHUFFLE_H_
#define BASE_FILE_DESCRIPTOR_SHUFFLE_H_

















#include <vector>

namespace base {



class InjectionDelegate {
 public:
  
  
  virtual bool Duplicate(int* result, int fd) = 0;
  
  
  virtual bool Move(int src, int dest) = 0;
  
  virtual void Close(int fd) = 0;
};



class FileDescriptorTableInjection : public InjectionDelegate {
  virtual bool Duplicate(int* result, int fd) MOZ_OVERRIDE;
  virtual bool Move(int src, int dest) MOZ_OVERRIDE;
  virtual void Close(int fd) MOZ_OVERRIDE;
};


struct InjectionArc {
  InjectionArc(int in_source, int in_dest, bool in_close)
      : source(in_source),
        dest(in_dest),
        close(in_close) {
  }

  int source;
  int dest;
  bool close;  
               
};

typedef std::vector<InjectionArc> InjectiveMultimap;

bool PerformInjectiveMultimap(const InjectiveMultimap& map,
                              InjectionDelegate* delegate);
bool PerformInjectiveMultimapDestructive(InjectiveMultimap* map,
                                         InjectionDelegate* delegate);


static inline bool ShuffleFileDescriptors(InjectiveMultimap *map) {
  FileDescriptorTableInjection delegate;
  return PerformInjectiveMultimapDestructive(map, &delegate);
}

}  

#endif  
