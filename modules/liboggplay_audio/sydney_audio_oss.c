


































#include "sydney_audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/soundcard.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define SA_READ_PERIOD 0 
#define SA_WRITE_PERIOD 2560 // 40 ms of 16-bit, stereo, 16kHz
#define SA_READ_BUFFER 0
#define SA_WRITE_BUFFER 7680 // 3 periods per buffer


#define OSS_VERSION(x, y, z) (x << 16 | y << 8 | z)

#define SUPP_OSS_VERSION OSS_VERSION(3,6,1)

#if (SOUND_VERSION >= SUPP_OSS_VERSION)

struct SAAudioHandle_ {
   char *device_name;
   int channels;
   int read_period;
   int write_period;
   int read_buffer;
   int write_buffer;
   sa_pcm_mode_t rw_mode;
   sa_pcm_format_t format;
   int rate;
   int interleaved;

   int capture_handle;
   int playback_handle;
   int readN, writeN;
   char *stored;
   int stored_amount;
   int stored_limit;
   
};



int sa_device_create_pcm(SAAudioHandle **_dev, const char *client_name, sa_pcm_mode_t rw_mode, sa_pcm_format_t format, int rate, int channels);

int sa_device_open(SAAudioHandle *dev);

int sa_device_close(SAAudioHandle *dev);



int sa_device_set_write_lower_watermark(SAAudioHandle *dev, int size);

int sa_device_set_read_lower_watermark(SAAudioHandle *dev, int size);

int sa_device_set_write_upper_watermark(SAAudioHandle *dev, int size);

int sa_device_set_read_upper_watermark(SAAudioHandle *dev, int size);


int sa_device_change_input_volume(SAAudioHandle *dev, const int *vol);

int sa_device_change_output_volume(SAAudioHandle *dev, const int *vol);


int sa_device_get_position(SAAudioHandle *dev, sa_pcm_index_t ref, int64_t *pos);



int sa_device_write(SAAudioHandle *dev, size_t nbytes, const void *data);



static int oss_audio_format(sa_pcm_format_t sa_format, int* oss_format);










int sa_device_create_pcm(SAAudioHandle **_dev, const char *client_name, sa_pcm_mode_t rw_mode, sa_pcm_format_t format, int rate, int channels) {
	SAAudioHandle* dev = NULL;
	
	dev = malloc(sizeof(SAAudioHandle));
	
	if (!dev) {
		return SA_DEVICE_OOM;	
	}		        
	dev->channels = channels;
	dev->format = format;
	dev->rw_mode = rw_mode;
	dev->rate = rate;
	dev->readN = 0;
	dev->readN = 0;
	dev->capture_handle = -1;
	dev->playback_handle = -1;
	dev->interleaved = 1;
	dev->read_period = SA_READ_PERIOD;
	dev->write_period = SA_WRITE_PERIOD;
	dev->read_buffer = SA_READ_BUFFER;
	dev->write_buffer = SA_WRITE_BUFFER;
  dev->device_name = "/dev/dsp";
  dev->stored = NULL;
  dev->stored_amount = 0;
  dev->stored_limit = 0;
	
	*_dev = dev;
	 
	 return SA_DEVICE_SUCCESS;
}






int sa_device_open(SAAudioHandle *dev) {	
 	int err;
	int fmt;
	int audio_fd = -1;

	if (dev->rw_mode == SA_PCM_WRONLY) {
		
		dev->device_name = "/dev/dsp"; 
		audio_fd = open(dev->device_name, O_WRONLY, 0);
		if (audio_fd == -1) {
		   fprintf(stderr, "Cannot open device: %s\n", dev->device_name);
		   
		   return SA_DEVICE_OOM;
		}

		
		if ((err = ioctl(audio_fd, SNDCTL_DSP_SPEED, &(dev->rate))) < 0) {
		   fprintf(stderr, 
			"Error setting the audio playback rate [%d]\n", 
			dev->rate);
		   
		   return SA_DEVICE_OOM; 
		}
		
		if ((err = ioctl(audio_fd,  SNDCTL_DSP_CHANNELS, 
			&(dev->channels)))< 0) {
		   fprintf(stderr, "Error setting audio channels\n");
		   
		   return SA_DEVICE_OOM;	
		} 
		if ((err = oss_audio_format(dev->format, &fmt)) < 0) {
		   fprintf(stderr, "Format unknown\n");
		   
                   return SA_DEVICE_OOM;	   
		}
		printf("Setting format with value %d\n", fmt);
		if ((err = ioctl(audio_fd, SNDCTL_DSP_SETFMT, &fmt)) < 0 ) {
		   fprintf(stderr, "Error setting audio format\n");
		   
		   return SA_DEVICE_OOM;
		}

		dev->playback_handle = audio_fd;
 		
	}
	if (dev->rw_mode == SA_PCM_RDONLY) {
		return SA_DEVICE_NOT_SUPPORTED;
	} 
	if (dev->rw_mode == SA_PCM_RW) {
		return SA_DEVICE_NOT_SUPPORTED;
	}
	fprintf(stderr, "Audio device opened successfully\n");
	return SA_DEVICE_SUCCESS;
}

#define WRITE(data,amt)                                       \
  if ((err = write(dev->playback_handle, data, amt)) < 0) {   \
    fprintf(stderr, "Error writing data to audio device\n");  \
    return SA_DEVICE_OOM;                                     \
  }








int sa_device_write(SAAudioHandle *dev, size_t nbytes, const void *_data) {
	int               err;
  audio_buf_info    info;
  int               bytes;
  char            * data = (char *)_data;
  







	if ((dev->playback_handle) > 0) {
    ioctl(dev->playback_handle, SNDCTL_DSP_GETOSPACE, &info);
    bytes = info.bytes;
    if (dev->stored_amount > bytes) {
      WRITE(dev->stored, bytes);
      memmove(dev->stored, dev->stored + bytes, dev->stored_amount - bytes);
      dev->stored_amount -= bytes;
    } else if (dev->stored_amount > 0) {
      WRITE(dev->stored, dev->stored_amount);
      bytes -= dev->stored_amount;
      dev->stored_amount = 0;
      if (nbytes < bytes) {
        WRITE(data, nbytes);
        return SA_DEVICE_SUCCESS;
      }
      WRITE(data, bytes);
      data += bytes;
      nbytes -= bytes;
    } else {
      if (nbytes < bytes) {
        WRITE(data, nbytes);
        return SA_DEVICE_SUCCESS;
      }
      WRITE(data, bytes);
      data += bytes;
      nbytes -= bytes;
    }

    if (nbytes > 0) {
      if (dev->stored_amount + nbytes > dev->stored_limit) {
        dev->stored = realloc(dev->stored, dev->stored_amount + nbytes);
      }
      
      memcpy(dev->stored + dev->stored_amount, data, nbytes);
      dev->stored_amount += nbytes;
    }
	}
	return SA_DEVICE_SUCCESS;
}

#define CLOSE_HANDLE(x) if (x != -1) close(x);






int sa_device_close(SAAudioHandle *dev) {
  int err;

	if (dev != NULL) {

    if (dev->stored_amount > 0) {
      WRITE(dev->stored, dev->stored_amount);
    }

    if (dev->stored != NULL) {
      free(dev->stored);
    }

    dev->stored = NULL;
    dev->stored_amount = 0;
    dev->stored_limit = 0;
	  
    CLOSE_HANDLE(dev->playback_handle);
	  CLOSE_HANDLE(dev->capture_handle);

	  printf("Closing audio device\n");	
	  free(dev);
	}
 	return SA_DEVICE_SUCCESS;
}







int sa_device_set_write_lower_watermark(SAAudioHandle *dev, int size) {
   dev->write_period = size;
   return SA_DEVICE_SUCCESS;
}






int sa_device_set_read_lower_watermark(SAAudioHandle *dev, int size) {
   dev->read_period = size;
   return SA_DEVICE_SUCCESS;
}






int sa_device_set_write_upper_watermark(SAAudioHandle *dev, int size) {  
   dev->write_buffer = size;
   return SA_DEVICE_SUCCESS;
}







int sa_device_set_read_upper_watermark(SAAudioHandle *dev, int size) {
   dev->read_buffer = size;
   return SA_DEVICE_SUCCESS;
}


int sa_device_set_xrun_mode(SAAudioHandle *dev, sa_xrun_mode_t mode) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_set_ni(SAAudioHandle *dev) {
   dev->interleaved = 1;
   return SA_DEVICE_SUCCESS;
}

int sa_device_start_thread(SAAudioHandle *dev, sa_device_callback *callback) {
   return SA_DEVICE_NOT_SUPPORTED;
}

int sa_device_set_channel_map(SAAudioHandle *dev, const sa_channel_def_t *map) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_change_device(SAAudioHandle *dev, const char *device_name) {
   return SA_DEVICE_NOT_SUPPORTED;
}







int sa_device_change_input_volume(SAAudioHandle *dev, const int *vol) {
#if SOUND_VERSION >= OSS_VERSION(4,0,0)	
	int err;
	if ((err = ioctl(dev->capture_handle, SNDCTL_DSP_SETRECVOL, vol) < 0) {
	   fpritnf(stderr, "Error setting new recording volume level\n");
	   
           return SA_DEVICE_OOM;	   
	}
	return SA_DEVICE_SUCCESS;
#else
	return SA_DEVICE_NOT_SUPPORTED;
#endif
}







int sa_device_change_output_volume(SAAudioHandle *dev, const int *vol) {
#if SOUND_VERSION >= OSS_VERSION(4,0,0)
	int err;
	if ((err = ioctl(dev->playback_handle, SNDCTL_DSP_SETPLAYVOL, vol) < 0){

	fprintf(stderr, "Error setting new playback volume\n");
	
	return SA_DEVICE_OOM; 
        }
	return SA_DEVICE_SUCCESS;
#else
	return SA_DEVICE_NOT_SUPPORTED;
#endif
}

int sa_device_change_sampling_rate(SAAudioHandle *dev, int rate) {
   dev->rate = rate;
   return SA_DEVICE_SUCCESS;
}

int sa_device_change_client_name(SAAudioHandle *dev, const char *client_name) {
   return SA_DEVICE_NOT_SUPPORTED;
}

int sa_device_change_stream_name(SAAudioHandle *dev, const char *stream_name) {
   return SA_DEVICE_NOT_SUPPORTED;
}

int sa_device_change_user_data(SAAudioHandle *dev, void *val) {
   return SA_DEVICE_NOT_SUPPORTED;
}









int sa_device_adjust_rate(SAAudioHandle *dev, int rate, int direction) {
   return  SA_DEVICE_NOT_SUPPORTED;
}






int sa_device_adjust_channels(SAAudioHandle *dev, int nb_channels) {               return SA_DEVICE_NOT_SUPPORTED;
}

int sa_device_adjust_format(SAAudioHandle *dev, sa_pcm_format_t format, int direction) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_state(SAAudioHandle *dev, sa_state_t *running) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_sampling_rate(SAAudioHandle *dev, int *rate) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_nb_channels(SAAudioHandle *dev, int *nb_channels) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_format(SAAudioHandle *dev, sa_pcm_format_t *format) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_user_data(SAAudioHandle *dev, void **val) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_event_error(SAAudioHandle *dev, sa_pcm_error_t *error) {
   return SA_DEVICE_NOT_SUPPORTED;
}


int sa_device_get_event_notify(SAAudioHandle *dev, sa_pcm_notification_t *notify) {
   return SA_DEVICE_NOT_SUPPORTED;
}








int sa_device_get_position(SAAudioHandle *dev, sa_pcm_index_t ref, int64_t *pos)
{
   int err;
   int64_t _pos;
   int delay;
   count_info ptr;
   switch (ref) {
	 case SA_PCM_WRITE_DELAY:
	      
	      if ((err = ioctl(dev->playback_handle, 
			       SNDCTL_DSP_GETODELAY, 
 			       &delay)) <0) {
	      	fprintf(stderr, "Error reading playback buffering delay\n");
		return SA_DEVICE_OOM;	
	      };  
	      _pos = (int64_t)delay;
	      break;
         case SA_PCM_WRITE_SOFTWARE_POS:
              
	      if ((err = ioctl(dev->playback_handle, 
                               SNDCTL_DSP_GETOPTR, 
                               &ptr)) <0) {
                
		return SA_DEVICE_OOM;
              };
	      _pos = (int64_t)ptr.bytes;  
	      break;
         case SA_PCM_READ_SOFTWARE_POS:
              
	      if ((err = ioctl(dev->playback_handle, 
                               SNDCTL_DSP_GETIPTR, 
                               &ptr)) <0) {
              	 fprintf(stderr, "Error reading audio capture position\n");
		 return SA_DEVICE_OOM;
	      };
               _pos = (int64_t)ptr.bytes;
	      break;

	 case SA_PCM_READ_DELAY:
	 case SA_PCM_READ_HARDWARE_POS:
	 case SA_PCM_WRITE_HARDWARE_POS:               
	 case SA_PCM_DUPLEX_DELAY:
	 default:
	      return SA_DEVICE_NOT_SUPPORTED;
	      break;		
   }
   (*pos) = _pos;
   return SA_DEVICE_SUCCESS;
}









static int oss_audio_format(sa_pcm_format_t sa_format, int* oss_format) {
#if SOUND_VERSION >= OSS_VERSION(4,0,0) 	
	int fmt = AFMT_UNDEF;
#else
	int fmt = -1;
#endif	
	switch (sa_format) {
                   case SA_PCM_UINT8:
			fmt = AFMT_U8;
			break;
                   case SA_PCM_ULAW:
			fmt = AFMT_MU_LAW;
			break;
                   case SA_PCM_ALAW:
			fmt = AFMT_A_LAW;
			break;
		   
                   case SA_PCM_S16_LE:
			fmt = AFMT_S16_LE;
			break;
		   
                   case SA_PCM_S16_BE:
			fmt = AFMT_S16_BE;
			break;
#if SOUND_VERSION >= OSS_VERSION(4,0,0)
		   
                   case SA_PCM_S24_LE:
			fmt = AFMT_S24_LE;
			break;
		   
		   case SA_PCM_S24_BE:
			fmt = AFMT_S24_BE;
			break;
		   
                   case SA_PCM_S32_LE:
			fmt = AFMT_S32_LE;
			break;
		   
                   case SA_PCM_S32_BE:
			fmt = AFMT_S32_BE;
			break; 
                   case SA_PCM_FLOAT32_NE:
			fmt = AFMT_FLOAT;
			break;
#endif
		   default:
			return SA_DEVICE_NOT_SUPPORTED;
			break;

                }
	(*oss_format) = fmt;
	return SA_DEVICE_SUCCESS;
}



























#endif 
