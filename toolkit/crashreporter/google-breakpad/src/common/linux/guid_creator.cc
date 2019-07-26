




























#include "common/linux/guid_creator.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>









class GUIDGenerator {
 public:
  static uint32_t BytesToUInt32(const uint8_t bytes[]) {
    return ((uint32_t) bytes[0]
            | ((uint32_t) bytes[1] << 8)
            | ((uint32_t) bytes[2] << 16)
            | ((uint32_t) bytes[3] << 24));
  }

  static void UInt32ToBytes(uint8_t bytes[], uint32_t n) {
    bytes[0] = n & 0xff;
    bytes[1] = (n >> 8) & 0xff;
    bytes[2] = (n >> 16) & 0xff;
    bytes[3] = (n >> 24) & 0xff;
  }

  static bool CreateGUID(GUID *guid) {
    InitOnce();
    guid->data1 = random();
    guid->data2 = (uint16_t)(random());
    guid->data3 = (uint16_t)(random());
    UInt32ToBytes(&guid->data4[0], random());
    UInt32ToBytes(&guid->data4[4], random());
    return true;
  }

 private:
  static void InitOnce() {
    pthread_once(&once_control, &InitOnceImpl);
  }

  static void InitOnceImpl() {
    srandom(time(NULL));
  }

  static pthread_once_t once_control;
};

pthread_once_t GUIDGenerator::once_control = PTHREAD_ONCE_INIT;

bool CreateGUID(GUID *guid) {
  return GUIDGenerator::CreateGUID(guid);
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
