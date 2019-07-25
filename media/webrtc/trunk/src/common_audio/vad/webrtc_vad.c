









#include <stdlib.h>
#include <string.h>

#include "webrtc_vad.h"
#include "vad_core.h"

static const int kInitCheck = 42;

size_t WebRtcVad_AssignSize() {
  return sizeof(VadInstT);
}

int WebRtcVad_Assign(void* memory, VadInst** handle) {
  if (handle == NULL || memory == NULL) {
    return -1;
  }

  *handle = (VadInst*) memory;
  return 0;
}

int WebRtcVad_Create(VadInst** handle) {
  VadInstT* self = NULL;

  if (handle == NULL) {
    return -1;
  }

  *handle = NULL;
  self = (VadInstT*) malloc(sizeof(VadInstT));
  *handle = (VadInst*) self;

  if (self == NULL) {
    return -1;
  }

  self->init_flag = 0;

  return 0;
}

int WebRtcVad_Free(VadInst* handle) {
  if (handle == NULL) {
    return -1;
  }

  free(handle);

  return 0;
}


int WebRtcVad_Init(VadInst* handle) {
  
  return WebRtcVad_InitCore((VadInstT*) handle);
}


int WebRtcVad_set_mode(VadInst* handle, int mode) {
  VadInstT* self = (VadInstT*) handle;

  if (handle == NULL) {
    return -1;
  }
  if (self->init_flag != kInitCheck) {
    return -1;
  }

  return WebRtcVad_set_mode_core(self, mode);
}

int16_t WebRtcVad_Process(VadInst* vad_inst, int16_t fs, int16_t* speech_frame,
                          int16_t frame_length)
{
    int16_t vad;
    VadInstT* vad_ptr;

    if (vad_inst == NULL)
    {
        return -1;
    }

    vad_ptr = (VadInstT*)vad_inst;
    if (vad_ptr->init_flag != kInitCheck)
    {
        return -1;
    }

    if (speech_frame == NULL)
    {
        return -1;
    }

    if (fs == 32000)
    {
        if ((frame_length != 320) && (frame_length != 640) && (frame_length != 960))
        {
            return -1;
        }
        vad = WebRtcVad_CalcVad32khz((VadInstT*)vad_inst, speech_frame, frame_length);

    } else if (fs == 16000)
    {
        if ((frame_length != 160) && (frame_length != 320) && (frame_length != 480))
        {
            return -1;
        }
        vad = WebRtcVad_CalcVad16khz((VadInstT*)vad_inst, speech_frame, frame_length);

    } else if (fs == 8000)
    {
        if ((frame_length != 80) && (frame_length != 160) && (frame_length != 240))
        {
            return -1;
        }
        vad = WebRtcVad_CalcVad8khz((VadInstT*)vad_inst, speech_frame, frame_length);

    } else
    {
        return -1; 
    }

    if (vad > 0)
    {
        return 1;
    } else if (vad == 0)
    {
        return 0;
    } else
    {
        return -1;
    }
}
