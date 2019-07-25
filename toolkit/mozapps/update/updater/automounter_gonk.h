





#ifndef AUTOMOUNTER_GONK_H__
#define AUTOMOUNTER_GONK_H__

typedef enum {
  ReadOnly,
  ReadWrite,
  Unknown
} MountAccess;









class GonkAutoMounter
{
public:
  GonkAutoMounter();
  ~GonkAutoMounter();

  const MountAccess GetAccess()
  {
    return mAccess;
  }

private:
  bool RemountSystem(MountAccess access);
  bool ForceRemountReadOnly();
  bool UpdateMountStatus();
  bool ProcessMount(const char *mount);
  bool MountSystem(unsigned long flags);
  void Reboot();

private:
  char *mDevice;
  MountAccess mAccess;
};

#endif 
