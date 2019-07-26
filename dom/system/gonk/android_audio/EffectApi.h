















#ifndef ANDROID_EFFECTAPI_H_
#define ANDROID_EFFECTAPI_H_

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#if __cplusplus
extern "C" {
#endif



























typedef struct effect_interface_s **effect_interface_t;



#define EFFECT_API_VERSION 0x0100 // Format 0xMMmm MM: Major version, mm: minor version


#define EFFECT_STRING_LEN_MAX 64











typedef struct effect_uuid_s {
    uint32_t timeLow;
    uint16_t timeMid;
    uint16_t timeHiAndVersion;
    uint16_t clockSeq;
    uint8_t node[6];
} effect_uuid_t;


#define EFFECT_UUID_INITIALIZER { 0xec7178ec, 0xe5e1, 0x4432, 0xa3f4, \
                                  { 0x46, 0x57, 0xe6, 0x79, 0x52, 0x10 } }
static const effect_uuid_t EFFECT_UUID_NULL_ = EFFECT_UUID_INITIALIZER;
const effect_uuid_t * const EFFECT_UUID_NULL = &EFFECT_UUID_NULL_;
const char * const EFFECT_UUID_NULL_STR = "ec7178ec-e5e1-4432-a3f4-4657e6795210";



typedef struct effect_descriptor_s {
    effect_uuid_t type;     
    effect_uuid_t uuid;     
    uint16_t apiVersion;    
    uint32_t flags;         
    uint16_t cpuLoad;       
    uint16_t memoryUsage;   
    char    name[EFFECT_STRING_LEN_MAX];   
    char    implementor[EFFECT_STRING_LEN_MAX];    
} effect_descriptor_t;




































































#define EFFECT_FLAG_TYPE_MASK           0x00000003
#define EFFECT_FLAG_TYPE_INSERT         0x00000000
#define EFFECT_FLAG_TYPE_AUXILIARY      0x00000001
#define EFFECT_FLAG_TYPE_REPLACE        0x00000002


#define EFFECT_FLAG_INSERT_MASK         0x0000001C
#define EFFECT_FLAG_INSERT_ANY          0x00000000
#define EFFECT_FLAG_INSERT_FIRST        0x00000004
#define EFFECT_FLAG_INSERT_LAST         0x00000008
#define EFFECT_FLAG_INSERT_EXCLUSIVE    0x0000000C



#define EFFECT_FLAG_VOLUME_MASK         0x00000060
#define EFFECT_FLAG_VOLUME_CTRL         0x00000020
#define EFFECT_FLAG_VOLUME_IND          0x00000040
#define EFFECT_FLAG_VOLUME_NONE         0x00000000


#define EFFECT_FLAG_DEVICE_MASK         0x00000180
#define EFFECT_FLAG_DEVICE_IND          0x00000080
#define EFFECT_FLAG_DEVICE_NONE         0x00000000


#define EFFECT_FLAG_INPUT_MASK          0x00000600
#define EFFECT_FLAG_INPUT_DIRECT        0x00000000
#define EFFECT_FLAG_INPUT_PROVIDER      0x00000200
#define EFFECT_FLAG_INPUT_BOTH          0x00000400


#define EFFECT_FLAG_OUTPUT_MASK          0x00001800
#define EFFECT_FLAG_OUTPUT_DIRECT        0x00000000
#define EFFECT_FLAG_OUTPUT_PROVIDER      0x00000800
#define EFFECT_FLAG_OUTPUT_BOTH          0x00001000


#define EFFECT_FLAG_HW_ACC_MASK          0x00006000
#define EFFECT_FLAG_HW_ACC_SIMPLE        0x00002000
#define EFFECT_FLAG_HW_ACC_TUNNEL        0x00004000


#define EFFECT_FLAG_AUDIO_MODE_MASK      0x00018000
#define EFFECT_FLAG_AUDIO_MODE_IND       0x00008000
#define EFFECT_FLAG_AUDIO_MODE_NONE      0x00000000


typedef struct audio_buffer_s audio_buffer_t;




































typedef int32_t (*effect_process_t)(effect_interface_t self,
                                    audio_buffer_t *inBuffer,
                                    audio_buffer_t *outBuffer);































typedef int32_t (*effect_command_t)(effect_interface_t self,
                                    uint32_t cmdCode,
                                    uint32_t cmdSize,
                                    void *pCmdData,
                                    uint32_t *replySize,
                                    void *pReplyData);



struct effect_interface_s {
    effect_process_t process;
    effect_command_t command;
};





enum effect_command_e {
   EFFECT_CMD_INIT,                 
   EFFECT_CMD_CONFIGURE,            
   EFFECT_CMD_RESET,                
   EFFECT_CMD_ENABLE,               
   EFFECT_CMD_DISABLE,              
   EFFECT_CMD_SET_PARAM,            
   EFFECT_CMD_SET_PARAM_DEFERRED,   
   EFFECT_CMD_SET_PARAM_COMMIT,     
   EFFECT_CMD_GET_PARAM,            
   EFFECT_CMD_SET_DEVICE,           
   EFFECT_CMD_SET_VOLUME,           
   EFFECT_CMD_SET_AUDIO_MODE,       
   EFFECT_CMD_FIRST_PROPRIETARY = 0x10000 
};































































































































































































struct audio_buffer_s {
    size_t   frameCount;        
    union {
        void*       raw;        
        int32_t*    s32;        
        int16_t*    s16;        
        uint8_t*    u8;         
    };
};













typedef int32_t (* buffer_function_t)(void *cookie, audio_buffer_t *buffer);

typedef struct buffer_provider_s {
    buffer_function_t getBuffer;       
    buffer_function_t releaseBuffer;   
    void       *cookie;                
} buffer_provider_t;






typedef struct buffer_config_s {
    audio_buffer_t  buffer;     
    uint32_t   samplingRate;    
    uint32_t   channels;        
    buffer_provider_t bufferProvider;   
    uint8_t    format;          
    uint8_t    accessMode;      
    uint16_t   mask;            
} buffer_config_t;


enum audio_format_e {
    SAMPLE_FORMAT_PCM_S15,   
    SAMPLE_FORMAT_PCM_U8,    
    SAMPLE_FORMAT_PCM_S7_24, 
    SAMPLE_FORMAT_OTHER      
};


enum audio_channels_e {
    CHANNEL_FRONT_LEFT = 0x1,                   
    CHANNEL_FRONT_RIGHT = 0x2,                  
    CHANNEL_FRONT_CENTER = 0x4,                
    CHANNEL_LOW_FREQUENCY = 0x8,               
    CHANNEL_BACK_LEFT = 0x10,                   
    CHANNEL_BACK_RIGHT = 0x20,                  
    CHANNEL_FRONT_LEFT_OF_CENTER = 0x40,       
    CHANNEL_FRONT_RIGHT_OF_CENTER = 0x80,      
    CHANNEL_BACK_CENTER = 0x100,                
    CHANNEL_MONO = CHANNEL_FRONT_LEFT,
    CHANNEL_STEREO = (CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT),
    CHANNEL_QUAD = (CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT |
            CHANNEL_BACK_LEFT | CHANNEL_BACK_RIGHT),
    CHANNEL_SURROUND = (CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT |
            CHANNEL_FRONT_CENTER | CHANNEL_BACK_CENTER),
    CHANNEL_5POINT1 = (CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT |
            CHANNEL_FRONT_CENTER | CHANNEL_LOW_FREQUENCY | CHANNEL_BACK_LEFT | CHANNEL_BACK_RIGHT),
    CHANNEL_7POINT1 = (CHANNEL_FRONT_LEFT | CHANNEL_FRONT_RIGHT |
            CHANNEL_FRONT_CENTER | CHANNEL_LOW_FREQUENCY | CHANNEL_BACK_LEFT | CHANNEL_BACK_RIGHT |
            CHANNEL_FRONT_LEFT_OF_CENTER | CHANNEL_FRONT_RIGHT_OF_CENTER),
};


enum audio_device_e {
    DEVICE_EARPIECE = 0x1,                      
    DEVICE_SPEAKER = 0x2,                       
    DEVICE_WIRED_HEADSET = 0x4,                 
    DEVICE_WIRED_HEADPHONE = 0x8,               
    DEVICE_BLUETOOTH_SCO = 0x10,                
    DEVICE_BLUETOOTH_SCO_HEADSET = 0x20,        
    DEVICE_BLUETOOTH_SCO_CARKIT = 0x40,         
    DEVICE_BLUETOOTH_A2DP = 0x80,               
    DEVICE_BLUETOOTH_A2DP_HEADPHONES = 0x100,   
    DEVICE_BLUETOOTH_A2DP_SPEAKER = 0x200,      
    DEVICE_AUX_DIGITAL = 0x400,                 
    DEVICE_EXTERNAL_SPEAKER = 0x800             
};


enum audio_mode_e {
    AUDIO_MODE_NORMAL,      
    AUDIO_MODE_RINGTONE,    
    AUDIO_MODE_IN_CALL      
};



enum effect_buffer_access_e {
    EFFECT_BUFFER_ACCESS_WRITE,
    EFFECT_BUFFER_ACCESS_READ,
    EFFECT_BUFFER_ACCESS_ACCUMULATE

};



#define EFFECT_CONFIG_BUFFER    0x0001  // buffer field must be taken into account
#define EFFECT_CONFIG_SMP_RATE  0x0002  // samplingRate field must be taken into account
#define EFFECT_CONFIG_CHANNELS  0x0004  // channels field must be taken into account
#define EFFECT_CONFIG_FORMAT    0x0008  // format field must be taken into account
#define EFFECT_CONFIG_ACC_MODE  0x0010  // accessMode field must be taken into account
#define EFFECT_CONFIG_PROVIDER  0x0020  // bufferProvider field must be taken into account
#define EFFECT_CONFIG_ALL (EFFECT_CONFIG_BUFFER | EFFECT_CONFIG_SMP_RATE | \
                           EFFECT_CONFIG_CHANNELS | EFFECT_CONFIG_FORMAT | \
                           EFFECT_CONFIG_ACC_MODE | EFFECT_CONFIG_PROVIDER)




typedef struct effect_config_s {
    buffer_config_t   inputCfg;
    buffer_config_t   outputCfg;;
} effect_config_t;


























typedef struct effect_param_s {
    int32_t     status;     
    uint32_t    psize;      
    uint32_t    vsize;      
    char        data[];     
} effect_param_t;



































typedef int32_t (*effect_QueryNumberEffects_t)(uint32_t *pNumEffects);




























typedef int32_t (*effect_QueryEffect_t)(uint32_t index,
                                        effect_descriptor_t *pDescriptor);






























typedef int32_t (*effect_CreateEffect_t)(effect_uuid_t *uuid,
                                         int32_t sessionId,
                                         int32_t ioId,
                                         effect_interface_t *pInterface);


















typedef int32_t (*effect_ReleaseEffect_t)(effect_interface_t interface);


#if __cplusplus
}  
#endif


#endif
