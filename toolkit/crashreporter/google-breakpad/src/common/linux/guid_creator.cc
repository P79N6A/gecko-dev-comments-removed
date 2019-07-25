




























#include "common/linux/guid_creator.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>









class GUIDGenerator {
 public:
  GUIDGenerator() {
    srandom(time(NULL));
  }

  static u_int32_t BytesToUInt32(const u_int8_t bytes[]) {
    return ((u_int32_t) bytes[0]
            | ((u_int32_t) bytes[1] << 8)
            | ((u_int32_t) bytes[2] << 16)
            | ((u_int32_t) bytes[3] << 24));
  }

  static void UInt32ToBytes(u_int8_t bytes[], u_int32_t n) {
    bytes[0] = n & 0xff;
    bytes[1] = (n >> 8) & 0xff;
    bytes[2] = (n >> 16) & 0xff;
    bytes[3] = (n >> 24) & 0xff;
  }

  bool CreateGUID(GUID *guid) const {
    guid->data1 = random();
    guid->data2 = (u_int16_t)(random());
    guid->data3 = (u_int16_t)(random());
    UInt32ToBytes(&guid->data4[0], random());
    UInt32ToBytes(&guid->data4[4], random());
    return true;
  }
};


const GUIDGenerator kGuidGenerator;

bool CreateGUID(GUID *guid) {
  return kGuidGenerator.CreateGUID(guid);
}


bool GUIDToString(const GUID *guid, char *buf, int buf_len) {
  
  assert(buf_len > kGUIDStringLength);
  int num = snprintf(buf, buf_len, kGUIDFormatString,
                     guid->data1, guid->data2, guid->data3,
                     GUIDGenerator::BytesToUInt32(&(guid->data4[0])),
                     GUIDGenerator::BytesToUInt32(&(guid->data4[4])));
  if (num != kGUIDStringLength)
    return false;

  buf[num] = '\0';
  return true;
}
