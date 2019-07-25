





































#ifndef TimingStruct_h_
#define TimingStruct_h_

struct TimingStruct {
  TimingStruct() {
    memset(this, 0, sizeof(TimingStruct));
  }

  PRTime channelCreation;
  PRTime asyncOpen;

  PRTime domainLookupStart;
  PRTime domainLookupEnd;
  PRTime connectStart;
  PRTime connectEnd;
  PRTime requestStart;
  PRTime responseStart;
  PRTime responseEnd;
};

#endif
