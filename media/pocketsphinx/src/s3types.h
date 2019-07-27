




































#ifndef _S3_S3TYPES_H_
#define _S3_S3TYPES_H_

#include <float.h>
#include <assert.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>





#ifdef __cplusplus
extern "C" {
#endif








typedef int16		s3cipid_t;	
#define BAD_S3CIPID	((s3cipid_t) -1)
#define NOT_S3CIPID(p)	((p)<0)
#define IS_S3CIPID(p)	((p)>=0)
#define MAX_S3CIPID	32767



typedef int32		s3pid_t;	
#define BAD_S3PID	((s3pid_t) -1)
#define NOT_S3PID(p)	((p)<0)
#define IS_S3PID(p)	((p)>=0)
#define MAX_S3PID	((int32)0x7ffffffe)

typedef uint16		s3ssid_t;	
#define BAD_S3SSID	((s3ssid_t) 0xffff)
#define NOT_S3SSID(p)	((p) == BAD_S3SSID)
#define IS_S3SSID(p)	((p) != BAD_S3SSID)
#define MAX_S3SSID	((s3ssid_t)0xfffe)

typedef int32		s3tmatid_t;	
#define BAD_S3TMATID	((s3tmatid_t) -1)
#define NOT_S3TMATID(t)	((t)<0)
#define IS_S3TMATID(t)	((t)>=0)
#define MAX_S3TMATID	((int32)0x7ffffffe)

typedef int32		s3wid_t;	
#define BAD_S3WID	((s3wid_t) -1)
#define NOT_S3WID(w)	((w)<0)
#define IS_S3WID(w)	((w)>=0)
#define MAX_S3WID	((int32)0x7ffffffe)

#ifdef __cplusplus
}
#endif

#endif
