






























#include <cassert>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common/solaris/guid_creator.h"








class GUIDGenerator {
 public:
  GUIDGenerator() {
    srandom(time(NULL));
  }

  bool CreateGUID(GUID *guid) const {
    guid->data1 = random();
    guid->data2 = (uint16_t)(random());
    guid->data3 = (uint16_t)(random());
    *reinterpret_cast<uint32_t*>(&guid->data4[0]) = random();
    *reinterpret_cast<uint32_t*>(&guid->data4[4]) = random();
    return true;
  }
};


const GUIDGenerator kGuidGenerator;

bool CreateGUID(GUID *guid) {
  return kGuidGenerator.CreateGUID(guid);
};


bool GUIDToString(const GUID *guid, char *buf, int buf_len) {
  
  assert(buf_len > kGUIDStringLength);
  int num = snprintf(buf, buf_len, kGUIDFormatString,
                     guid->data1, guid->data2, guid->data3,
                     *reinterpret_cast<const uint32_t *>(&(guid->data4[0])),
                     *reinterpret_cast<const uint32_t *>(&(guid->data4[4])));
  if (num != kGUIDStringLength)
    return false;

  buf[num] = '\0';
  return true;
}
