
















































#import <Foundation/Foundation.h>

#define SMSLIB_VERSION "1.8"

#pragma mark Structure definitions



typedef struct sms_acceleration {
	float x;		
	float y;		
	float z;		
} sms_acceleration;


typedef struct sms_calibration {
	float zeros[3];	
	float onegs[3];	
} sms_calibration;

#pragma mark Return value definitions






#define SMS_FAIL_MODEL			(-7)

#define SMS_FAIL_DICTIONARY		(-6)

#define SMS_FAIL_LIST_SERVICES	(-5)


#define SMS_FAIL_NO_SERVICES	(-4)

#define SMS_FAIL_OPENING		(-3)

#define SMS_FAIL_CONNECTION		(-2)



#define SMS_FAIL_ACCESS			(-1)

#define SMS_SUCCESS				(0)

#pragma mark Function declarations













int smsStartup(id logObject, SEL logSelector);







int smsDebugStartup(id logObject, SEL logSelector);


void smsGetCalibration(sms_calibration *calibrationRecord);



void smsSetCalibration(sms_calibration *calibrationRecord);


void smsStoreCalibration(void);



BOOL smsLoadCalibration(void);



void smsDeleteCalibration(void);



int smsGetData(sms_acceleration *accel);



int smsGetUncalibratedData(sms_acceleration *accel);


int smsGetBufferLength(void);



void smsGetBufferData(char *buffer);



NSString *smsGetCalibrationDescription(void);


void smsShutdown(void);

