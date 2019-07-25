


























































#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sydney_audio.h"

#define INCL_DOS
#define INCL_MCIOS2
#include <os2.h>
#include <os2me.h>
#include <386/builtin.h>





#define SAOS2_SAMPLE_SIZE   2




#define SAOS2_BUF_CNT       40





#define SAOS2_MS_PER_WRITE  40



#define SAOS2_UNDERRUN_CNT  2



#define SAOS2_WAIT          5000




#ifdef DEBUG
  #ifndef SAOS2_ERROR
    #define SAOS2_ERROR
  #endif
#endif

#ifdef SAOS2_ERROR
  static int  os2_error_msg(int rtn, char * func, char * msg, uint32_t err);
  #define os2_error(rtn, func, msg, err)    os2_error_msg(rtn, func, msg, err)
#else
  #define os2_error(rtn, func, msg, err)    rtn
#endif




struct sa_stream {

  
  const char *      client_name;
  sa_mode_t         mode;
  sa_pcm_format_t   format;
  uint32_t          rate;
  uint32_t          nchannels;
  uint32_t          bps;

  
  uint16_t          hwDeviceID;
  uint32_t          hwMixHandle;
  PMIXERPROC        hwWriteProc;

  
  int32_t           bufCnt;
  size_t            bufSize;
  size_t            bufMin;
  PMCI_MIX_BUFFER   bufList;

  
  volatile uint32_t freeNew;
  int32_t           freeCnt;
  int32_t           freeNdx;
  volatile uint32_t usedNew;
  int32_t           usedCnt;
  int32_t           usedMin;

  
  volatile uint32_t playing;
  volatile uint32_t writeTime;
  volatile uint32_t writeNew;
  int64_t           writePos;
};




static int32_t  os2_mixer_event(uint32_t ulStatus, PMCI_MIX_BUFFER pBuffer,
                                uint32_t ulFlags);
static void     os2_stop_device(uint16_t hwDeviceID);
static int      os2_pause_device(uint16_t hwDeviceID, uint32_t release);
static int      os2_get_free_count(sa_stream_t *s, int32_t count);





static int      os2_load_mdm(void);


typedef ULONG _System     MCISENDCOMMAND(USHORT, USHORT, ULONG, PVOID, USHORT);
static MCISENDCOMMAND *   _mciSendCommand = 0;







int     sa_stream_create_pcm(sa_stream_t **  s, 
                             const char *    client_name, 
                             sa_mode_t       mode, 
                             sa_pcm_format_t format, 
                             unsigned int    rate, 
                             unsigned int    nchannels)
{
  uint32_t      status = SA_SUCCESS;
  uint32_t      size;
  uint32_t      rc;
  sa_stream_t * sTemp = 0;

  

do {
  
  if (os2_load_mdm() != SA_SUCCESS)
    return SA_ERROR_SYSTEM;

  if (mode != SA_MODE_WRONLY || format != SA_PCM_FORMAT_S16_LE)
    return os2_error(SA_ERROR_NOT_SUPPORTED, "sa_stream_create_pcm",
                     "invalid mode or format", 0);

  if (!s)
    return os2_error(SA_ERROR_INVALID, "sa_stream_create_pcm",
                     "s is null", 0);
  *s = 0;

  

  size = sizeof(sa_stream_t) + sizeof(PMCI_MIX_BUFFER) * SAOS2_BUF_CNT;
  rc = DosAllocMem((void**)&sTemp, size,
                   PAG_COMMIT | PAG_READ | PAG_WRITE);
  if (rc) {
    status = os2_error(SA_ERROR_OOM, "sa_stream_create_pcm",
                       "DosAllocMem - rc=", rc);
    break;
  }

  memset(sTemp, 0, size);
  sTemp->bufList = (PMCI_MIX_BUFFER)&sTemp[1];

  
  sTemp->client_name = client_name;
  sTemp->mode        = mode;
  sTemp->format      = format;
  sTemp->rate        = rate;
  sTemp->nchannels   = nchannels;
  sTemp->bps         = rate * nchannels * SAOS2_SAMPLE_SIZE;

  

  sTemp->bufCnt  = SAOS2_BUF_CNT;

  

  sTemp->bufMin  = (sTemp->bps * SAOS2_MS_PER_WRITE) / 1000;

  



  sTemp->bufSize = (((3 * sTemp->bufMin) / 2) + 0xfff) & ~0xfff;
  sTemp->bufSize -= sTemp->bufSize % (SAOS2_SAMPLE_SIZE * nchannels);

  *s = sTemp;

} while (0);

  
  if (status != SA_SUCCESS && sTemp) {
    if (sTemp)
      DosFreeMem(sTemp);
  }

  return status;
}





int     sa_stream_open(sa_stream_t *s)
{
  int                 status = SA_SUCCESS;
  uint32_t            rc;
  int32_t             ctr;
  uint32_t            bufCntRequested;
  MCI_AMP_OPEN_PARMS  AmpOpenParms;
  MCI_MIXSETUP_PARMS  MixSetupParms;
  MCI_BUFFER_PARMS    BufferParms;

  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_open", "s is null", 0);

do {
  
  bufCntRequested = s->bufCnt;
  s->bufCnt = 0;

  
  memset(&AmpOpenParms, 0, sizeof(MCI_AMP_OPEN_PARMS));
  AmpOpenParms.pszDeviceType = (PSZ)(MCI_DEVTYPE_AUDIO_AMPMIX | 0);

  rc = _mciSendCommand(0, MCI_OPEN,
                      MCI_WAIT | MCI_OPEN_TYPE_ID | MCI_OPEN_SHAREABLE,
                      (void*)&AmpOpenParms, 0);
  if (LOUSHORT(rc)) {
    status = os2_error(SA_ERROR_NO_DEVICE, "sa_stream_open",
                       "MCI_OPEN - rc=", LOUSHORT(rc));
    break;
  }

  
  s->hwDeviceID = AmpOpenParms.usDeviceID;

  
  memset(&MixSetupParms, 0, sizeof(MCI_MIXSETUP_PARMS));
  MixSetupParms.ulBitsPerSample = 16;
  MixSetupParms.ulFormatTag     = MCI_WAVE_FORMAT_PCM;
  MixSetupParms.ulFormatMode    = MCI_PLAY;
  MixSetupParms.ulSamplesPerSec = s->rate;
  MixSetupParms.ulChannels      = s->nchannels;
  MixSetupParms.ulDeviceType    = MCI_DEVTYPE_WAVEFORM_AUDIO;
  MixSetupParms.pmixEvent       = (MIXEREVENT*)os2_mixer_event;

  rc = _mciSendCommand(s->hwDeviceID, MCI_MIXSETUP,
                      MCI_WAIT | MCI_MIXSETUP_INIT,
                      (void*)&MixSetupParms, 0);
  if (LOUSHORT(rc)) {
    status = os2_error(SA_ERROR_NOT_SUPPORTED, "sa_stream_open",
                       "MCI_MIXSETUP - rc=", LOUSHORT(rc));
    break;
  }

  
  s->hwMixHandle = MixSetupParms.ulMixHandle;
  s->hwWriteProc = MixSetupParms.pmixWrite;

  
  BufferParms.ulStructLength = sizeof(MCI_BUFFER_PARMS);
  BufferParms.ulNumBuffers   = bufCntRequested;
  BufferParms.ulBufferSize   = s->bufSize;
  BufferParms.pBufList       = s->bufList;

  rc = _mciSendCommand(s->hwDeviceID, MCI_BUFFER,
                      MCI_WAIT | MCI_ALLOCATE_MEMORY,
                      (void*)&BufferParms, 0);
  if (LOUSHORT(rc)) {
    status = os2_error(SA_ERROR_OOM, "sa_stream_open",
                       "MCI_ALLOCATE_MEMORY - rc=", LOUSHORT(rc));
    break;
  }

  

  s->bufCnt  = BufferParms.ulNumBuffers;
  s->freeCnt = BufferParms.ulNumBuffers;

  
  s->usedMin = SAOS2_UNDERRUN_CNT;
  for (ctr = 0; ctr < s->bufCnt; ctr++) {
    s->bufList[ctr].ulStructLength = sizeof(MCI_MIX_BUFFER);
    s->bufList[ctr].ulBufferLength = 0;
    s->bufList[ctr].ulUserParm = (uint32_t)s;
  }

} while (0);

  return status;
}





int     sa_stream_destroy(sa_stream_t *s)
{
  int               status = SA_SUCCESS;
  uint32_t          rc;
  MCI_GENERIC_PARMS GenericParms = { 0 };
  MCI_BUFFER_PARMS  BufferParms;

  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_destroy", "s is null", 0);

  
  if (s->hwDeviceID) {

    
    s->bufMin = 0;
    s->playing = FALSE;

    


    rc = _mciSendCommand(s->hwDeviceID, MCI_ACQUIREDEVICE,
                         MCI_WAIT,
                         (void*)&GenericParms, 0);
    if (LOUSHORT(rc))
      os2_error(0, "sa_stream_destroy",
                "MCI_ACQUIREDEVICE - rc=", LOUSHORT(rc));

    
    os2_stop_device(s->hwDeviceID);

    
    if (s->bufCnt) {
      BufferParms.hwndCallback   = 0;
      BufferParms.ulStructLength = sizeof(MCI_BUFFER_PARMS);
      BufferParms.ulNumBuffers   = s->bufCnt;
      BufferParms.ulBufferSize   = s->bufSize;
      BufferParms.pBufList       = s->bufList;

      rc = _mciSendCommand(s->hwDeviceID, MCI_BUFFER,
                          MCI_WAIT | MCI_DEALLOCATE_MEMORY,
                          (void*)&BufferParms, 0);
      if (LOUSHORT(rc))
        status = os2_error(SA_ERROR_SYSTEM, "sa_stream_destroy",
                           "MCI_DEALLOCATE_MEMORY - rc=", LOUSHORT(rc));
    }

    rc = _mciSendCommand(s->hwDeviceID, MCI_CLOSE,
                        MCI_WAIT,
                        (void*)&GenericParms, 0);
    if (LOUSHORT(rc))
      status = os2_error(SA_ERROR_SYSTEM, "sa_stream_destroy",
                         "MCI_CLOSE - rc=", LOUSHORT(rc));
  }

  
  DosFreeMem(s);

  return status;
}





int     sa_stream_write(sa_stream_t * s, const void * data, size_t nbytes)
{
  uint32_t        rc;
  size_t          cnt;
  PMCI_MIX_BUFFER pHW;

  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_write", "s is null", 0);
  if (!data)
    return os2_error(SA_ERROR_INVALID, "sa_stream_write", "data is null", 0);

  
  if (!nbytes)
    return SA_SUCCESS;

  

  while (nbytes) {
    size_t  offs;
    size_t  left;

    

    if (os2_get_free_count(s, 1))
      return SA_ERROR_SYSTEM;

    
    pHW = &(s->bufList[s->freeNdx]);

    offs = pHW->ulBufferLength;
    left = s->bufSize - offs;
    cnt = (nbytes > left) ? left : nbytes;
    memcpy(&((char*)pHW->pBuffer)[offs], (char*)data, cnt);

    pHW->ulBufferLength += cnt;
    nbytes -= cnt;
    data = (char*)data + cnt;

    
    if (pHW->ulBufferLength < s->bufMin)
      continue;

    
    rc = s->hwWriteProc(s->hwMixHandle, pHW, 1);
    if (LOUSHORT(rc)) {
      pHW->ulBufferLength = 0;
      return os2_error(SA_ERROR_SYSTEM, "sa_stream_write",
                       "mixWrite - rc=", LOUSHORT(rc));
    }

    
    __atomic_increment(&s->usedNew);
    s->playing = TRUE;

    s->freeCnt--;
    s->freeNdx = (s->freeNdx + 1) % s->bufCnt;
  }

  return SA_SUCCESS;
}





int     sa_stream_get_position(sa_stream_t *s, sa_position_t position, int64_t *pos)
{
  uint32_t      rc;
  uint32_t      then;
  uint32_t      now;

  if (!s || !pos)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_get_position",
                     "s or pos is null", 0);

  if (position != SA_POSITION_WRITE_SOFTWARE)
    return os2_error(SA_ERROR_NOT_SUPPORTED, "sa_stream_get_position",
                     "unsupported postion type=", position);

  






  do {
    then = s->writeTime;
    s->writePos += __atomic_xchg(&s->writeNew, 0);
    *pos = s->writePos;

    
    if (s->playing && s->writePos) {
      DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &now, sizeof(now));
      *pos += ((now - then) * s->bps) / 1000;
    }
  } while (then != s->writeTime);

  return SA_SUCCESS;
}





int     sa_stream_resume(sa_stream_t *s)
{
  uint32_t          rc;
  MCI_GENERIC_PARMS GenericParms = { 0 };

  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_resume",
                     "s is null", 0);

  rc = _mciSendCommand(s->hwDeviceID, MCI_ACQUIREDEVICE,
                      MCI_WAIT,
                      (void*)&GenericParms, 0);
  if (LOUSHORT(rc))
    return os2_error(SA_ERROR_SYSTEM, "sa_stream_resume",
                     "MCI_ACQUIREDEVICE - rc=", LOUSHORT(rc));

  

  rc = _mciSendCommand(s->hwDeviceID, MCI_RESUME,
                      MCI_WAIT,
                      (void*)&GenericParms, 0);
  if (LOUSHORT(rc))
    os2_error(SA_ERROR_SYSTEM, "sa_stream_resume",
              "MCI_RESUME - rc=", LOUSHORT(rc));

  
  DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT,
                  (void*)&s->writeTime, sizeof(s->writeTime));
  s->playing = TRUE;

  return SA_SUCCESS;
}





int     sa_stream_pause(sa_stream_t *s)
{
  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_pause", "s is null", 0);

  
  s->playing = FALSE;
  return os2_pause_device(s->hwDeviceID, TRUE);
}





int     sa_stream_drain(sa_stream_t *s)
{
  int       status = SA_SUCCESS;
  char      buf[32];

  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_drain", "s is null", 0);

  
  s->usedMin = 0;

  

  memset(buf, 0, sizeof(buf));
  s->bufMin = 0;
  sa_stream_write(s, buf, s->nchannels * SAOS2_SAMPLE_SIZE);

  

  if (!s->writePos)
    s->writePos += __atomic_xchg(&s->writeNew, 0);
  if (!s->writePos)
    sa_stream_write(s, buf, s->nchannels * SAOS2_SAMPLE_SIZE);

  
  if (!status)
    status = os2_get_free_count(s, s->bufCnt);
  s->playing = FALSE;

  
  os2_stop_device(s->hwDeviceID);

  return status;
}





int     sa_stream_get_write_size(sa_stream_t *s, size_t *size)
{
  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_get_write_size",
                     "s is null", 0);

  

  if (os2_get_free_count(s, 0)) {
    *size = s->bufSize;
    return SA_ERROR_SYSTEM;
  }

  *size = s->freeCnt * s->bufSize;

  return SA_SUCCESS;
}





int     sa_stream_set_volume_abs(sa_stream_t *s, float vol)
{
  uint32_t      rc;
  MCI_SET_PARMS SetParms;

  if (!s)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_set_volume_abs",
                     "s is null", 0);

  

  SetParms.ulLevel = (vol * 100);
  SetParms.ulAudio = MCI_SET_AUDIO_ALL;

  rc = _mciSendCommand(s->hwDeviceID, MCI_SET,
                      MCI_WAIT | MCI_SET_AUDIO | MCI_SET_VOLUME,
                      (void*)&SetParms, 0);
  if (LOUSHORT(rc))
    return os2_error(SA_ERROR_SYSTEM, "sa_stream_set_volume_abs",
                     "MCI_SET_VOLUME - rc=", LOUSHORT(rc));

  return SA_SUCCESS;
}





int     sa_stream_get_volume_abs(sa_stream_t *s, float *vol)
{
  int              status = SA_SUCCESS;
  uint32_t         rc;
  MCI_STATUS_PARMS StatusParms;

  if (!s || !vol)
    return os2_error(SA_ERROR_NO_INIT, "sa_stream_get_volume_abs",
                     "s or vol is null", 0);

  memset(&StatusParms, 0, sizeof(MCI_STATUS_PARMS));
  StatusParms.ulItem = MCI_STATUS_VOLUME;

  rc = _mciSendCommand(s->hwDeviceID, MCI_STATUS,
                      MCI_WAIT | MCI_STATUS_ITEM,
                      (void*)&StatusParms, 0);
  if (LOUSHORT(rc)) {
    
    StatusParms.ulReturn = (50 | 50 << 16);
    status = os2_error(SA_ERROR_SYSTEM, "sa_stream_get_volume_abs",
                       "MCI_STATUS_VOLUME - rc=", LOUSHORT(rc));
  }

  



  *vol = (LOUSHORT(StatusParms.ulReturn) +
          HIUSHORT(StatusParms.ulReturn)) / 200.0;

  return status;
}








static int32_t os2_mixer_event(uint32_t ulStatus, PMCI_MIX_BUFFER pBuffer,
                               uint32_t ulFlags)
{
  sa_stream_t * s;

  
  if (ulFlags & MIX_STREAM_ERROR)
    os2_error(0, "os2_mixer_event", "MIX_STREAM_ERROR - status=", ulStatus);

  if (!(ulFlags & MIX_WRITE_COMPLETE))
    return os2_error(TRUE, "os2_mixer_event",
                     "unexpected event - flag=", ulFlags);

  if (!pBuffer || !pBuffer->ulUserParm)
    return os2_error(TRUE, "os2_mixer_event", "null pointer", 0);

  

  s = (sa_stream_t *)pBuffer->ulUserParm;

  
  s->usedCnt += __atomic_xchg(&s->usedNew, 0);
  s->usedCnt--;

  

  if (s->usedCnt < s->usedMin) {
    s->playing = FALSE;
    os2_pause_device(s->hwDeviceID, FALSE);
    os2_error(0, "os2_mixer_event",
              "too few buffers in use - recovering", 0);
  }

  


  __atomic_add(&s->writeNew, pBuffer->ulBufferLength);
  pBuffer->ulBufferLength = 0;
  DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT,
                  (void*)&s->writeTime, sizeof(s->writeTime));

  
  __atomic_increment(&s->freeNew);

  return TRUE;
}





static void os2_stop_device(uint16_t hwDeviceID)
{
  uint32_t          rc;
  MCI_GENERIC_PARMS GenericParms = { 0 };

  rc = _mciSendCommand(hwDeviceID, MCI_STOP,
                      MCI_WAIT,
                      (void*)&GenericParms, 0);
  if (LOUSHORT(rc))
    os2_error(0, "os2_stop_device", "MCI_STOP - rc=", LOUSHORT(rc));

  return;
}





static int  os2_pause_device(uint16_t hwDeviceID, uint32_t release)
{
  uint32_t          rc;
  MCI_GENERIC_PARMS GenericParms = { 0 };

  rc = _mciSendCommand(hwDeviceID, MCI_PAUSE,
                      MCI_WAIT,
                      (void*)&GenericParms, 0);
  if (LOUSHORT(rc))
    return os2_error(SA_ERROR_SYSTEM, "os2_pause_device",
                     "MCI_PAUSE - rc=", LOUSHORT(rc));

  if (release)
    _mciSendCommand(hwDeviceID, MCI_RELEASEDEVICE,
                   MCI_WAIT,
                   (void*)&GenericParms, 0);

  return SA_SUCCESS;
}





static int  os2_get_free_count(sa_stream_t *s, int32_t count)
{
  uint32_t  timeout = 0;

  while (1) {
    uint32_t now;

    s->freeCnt += __atomic_xchg(&s->freeNew, 0);
    if (s->freeCnt >= count)
      break;

    
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &now, sizeof(now));
    if (!timeout)
      timeout = now + SAOS2_WAIT;

    if (now > timeout)
      return os2_error(SA_ERROR_SYSTEM, "os2_get_free_count",
                       "timed-out waiting for free buffer(s)", 0);

    DosSleep(1);
  }

  return SA_SUCCESS;
}



#ifdef SAOS2_ERROR



static int  os2_error_msg(int rtn, char * func, char * msg, uint32_t err)
{
  if (!err)
    fprintf(stderr, "sa_os2 error - %s:  %s\n", func, msg);
  else
    fprintf(stderr, "sa_os2 error - %s:  %s %u\n", func, msg, err);
  fflush(stderr);

  return rtn;
}

#endif







static int  os2_load_mdm(void)
{
  uint32_t  rc;
  HMODULE   hmod;
  char      text[32];

  if (_mciSendCommand)
    return SA_SUCCESS;

  rc = DosLoadModule(text, sizeof(text), "MDM", &hmod);
  if (rc)
    return os2_error(SA_ERROR_SYSTEM, "os2_load_mdm",
                     "DosLoadModule - rc=", rc);

  
  rc = DosQueryProcAddr(hmod, 1, 0, (PFN*)&_mciSendCommand);
  if (rc) {
    _mciSendCommand = 0;
    return os2_error(SA_ERROR_SYSTEM, "os2_load_mdm",
                     "DosQueryProcAddr - rc=", rc);
  }

  return SA_SUCCESS;
}





#define UNSUPPORTED(func)   func { return SA_ERROR_NOT_SUPPORTED; }

UNSUPPORTED(int sa_stream_create_opaque(sa_stream_t **s, const char *client_name, sa_mode_t mode, const char *codec))
UNSUPPORTED(int sa_stream_set_write_lower_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_read_lower_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_write_upper_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_read_upper_watermark(sa_stream_t *s, size_t size))
UNSUPPORTED(int sa_stream_set_channel_map(sa_stream_t *s, const sa_channel_t map[], unsigned int n))
UNSUPPORTED(int sa_stream_set_xrun_mode(sa_stream_t *s, sa_xrun_mode_t mode))
UNSUPPORTED(int sa_stream_set_non_interleaved(sa_stream_t *s, int enable))
UNSUPPORTED(int sa_stream_set_dynamic_rate(sa_stream_t *s, int enable))
UNSUPPORTED(int sa_stream_set_driver(sa_stream_t *s, const char *driver))
UNSUPPORTED(int sa_stream_start_thread(sa_stream_t *s, sa_event_callback_t callback))
UNSUPPORTED(int sa_stream_stop_thread(sa_stream_t *s))
UNSUPPORTED(int sa_stream_change_device(sa_stream_t *s, const char *device_name))
UNSUPPORTED(int sa_stream_change_read_volume(sa_stream_t *s, const int32_t vol[], unsigned int n))
UNSUPPORTED(int sa_stream_change_write_volume(sa_stream_t *s, const int32_t vol[], unsigned int n))
UNSUPPORTED(int sa_stream_change_rate(sa_stream_t *s, unsigned int rate))
UNSUPPORTED(int sa_stream_change_meta_data(sa_stream_t *s, const char *name, const void *data, size_t size))
UNSUPPORTED(int sa_stream_change_user_data(sa_stream_t *s, const void *value))
UNSUPPORTED(int sa_stream_set_adjust_rate(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_set_adjust_nchannels(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_set_adjust_pcm_format(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_set_adjust_watermarks(sa_stream_t *s, sa_adjust_t direction))
UNSUPPORTED(int sa_stream_get_mode(sa_stream_t *s, sa_mode_t *access_mode))
UNSUPPORTED(int sa_stream_get_codec(sa_stream_t *s, char *codec, size_t *size))
UNSUPPORTED(int sa_stream_get_pcm_format(sa_stream_t *s, sa_pcm_format_t *format))
UNSUPPORTED(int sa_stream_get_rate(sa_stream_t *s, unsigned int *rate))
UNSUPPORTED(int sa_stream_get_nchannels(sa_stream_t *s, int *nchannels))
UNSUPPORTED(int sa_stream_get_user_data(sa_stream_t *s, void **value))
UNSUPPORTED(int sa_stream_get_write_lower_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_read_lower_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_write_upper_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_read_upper_watermark(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_channel_map(sa_stream_t *s, sa_channel_t map[], unsigned int *n))
UNSUPPORTED(int sa_stream_get_xrun_mode(sa_stream_t *s, sa_xrun_mode_t *mode))
UNSUPPORTED(int sa_stream_get_non_interleaved(sa_stream_t *s, int *enabled))
UNSUPPORTED(int sa_stream_get_dynamic_rate(sa_stream_t *s, int *enabled))
UNSUPPORTED(int sa_stream_get_driver(sa_stream_t *s, char *driver_name, size_t *size))
UNSUPPORTED(int sa_stream_get_device(sa_stream_t *s, char *device_name, size_t *size))
UNSUPPORTED(int sa_stream_get_read_volume(sa_stream_t *s, int32_t vol[], unsigned int *n))
UNSUPPORTED(int sa_stream_get_write_volume(sa_stream_t *s, int32_t vol[], unsigned int *n))
UNSUPPORTED(int sa_stream_get_meta_data(sa_stream_t *s, const char *name, void*data, size_t *size))
UNSUPPORTED(int sa_stream_get_adjust_rate(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_adjust_nchannels(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_adjust_pcm_format(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_adjust_watermarks(sa_stream_t *s, sa_adjust_t *direction))
UNSUPPORTED(int sa_stream_get_state(sa_stream_t *s, sa_state_t *state))
UNSUPPORTED(int sa_stream_get_event_error(sa_stream_t *s, sa_error_t *error))
UNSUPPORTED(int sa_stream_get_event_notify(sa_stream_t *s, sa_notify_t *notify))
UNSUPPORTED(int sa_stream_read(sa_stream_t *s, void *data, size_t nbytes))
UNSUPPORTED(int sa_stream_read_ni(sa_stream_t *s, unsigned int channel, void *data, size_t nbytes))
UNSUPPORTED(int sa_stream_write_ni(sa_stream_t *s, unsigned int channel, const void *data, size_t nbytes))
UNSUPPORTED(int sa_stream_pwrite(sa_stream_t *s, const void *data, size_t nbytes, int64_t offset, sa_seek_t whence))
UNSUPPORTED(int sa_stream_pwrite_ni(sa_stream_t *s, unsigned int channel, const void *data, size_t nbytes, int64_t offset, sa_seek_t whence))
UNSUPPORTED(int sa_stream_get_read_size(sa_stream_t *s, size_t *size))
UNSUPPORTED(int sa_stream_get_min_write(sa_stream_t *s, size_t *samples))

const char *sa_strerror(int code) { return NULL; }



