









#ifndef NETEQTEST_DUMMYRTPPACKET_H
#define NETEQTEST_DUMMYRTPPACKET_H

#include "NETEQTEST_RTPpacket.h"

class NETEQTEST_DummyRTPpacket : public NETEQTEST_RTPpacket {
 public:
  virtual int readFromFile(FILE* fp) OVERRIDE;
  virtual int writeToFile(FILE* fp) OVERRIDE;
  virtual void parseHeader() OVERRIDE;
};

#endif  
