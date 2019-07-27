




#ifndef MOZ_PROFILE_BUFFER_H
#define MOZ_PROFILE_BUFFER_H

#include "ProfileEntry.h"
#include "platform.h"
#include "ProfileJSONWriter.h"
#include "mozilla/RefPtr.h"

class ProfileBuffer : public mozilla::RefCounted<ProfileBuffer> {
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(ProfileBuffer)

  explicit ProfileBuffer(int aEntrySize);

  virtual ~ProfileBuffer();

  void addTag(const ProfileEntry& aTag);
  void StreamSamplesToJSON(SpliceableJSONWriter& aWriter, int aThreadId, double aSinceTime,
                           JSRuntime* rt, UniqueStacks& aUniqueStacks);
  void StreamMarkersToJSON(SpliceableJSONWriter& aWriter, int aThreadId, double aSinceTime,
                           UniqueStacks& aUniqueStacks);
  void DuplicateLastSample(int aThreadId);

  void addStoredMarker(ProfilerMarker* aStoredMarker);

  
  void deleteExpiredStoredMarkers();
  void reset();

protected:
  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff);
  int FindLastSampleOfThread(int aThreadId);

public:
  
  mozilla::UniquePtr<ProfileEntry[]> mEntries;

  
  
  int mWritePos;

  
  int mReadPos;

  
  int mEntrySize;

  
  uint32_t mGeneration;

  
  ProfilerMarkerLinkedList mStoredMarkers;
};

#endif
