









#ifndef VP9_ENCODER_VP9_ETHREAD_H_
#define VP9_ENCODER_VP9_ETHREAD_H_

struct VP9_COMP;
struct ThreadData;

typedef struct EncWorkerData {
  struct VP9_COMP *cpi;
  struct ThreadData *td;
  int start;
} EncWorkerData;

void vp9_encode_tiles_mt(struct VP9_COMP *cpi);

#endif  
