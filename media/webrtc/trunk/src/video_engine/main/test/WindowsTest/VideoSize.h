









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_WINDOWSTEST_VIDEOSIZE_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_WINDOWSTEST_VIDEOSIZE_H_
#include "StdAfx.h"
enum VideoSize
	{
		UNDEFINED, 
		SQCIF,     
		QQVGA,     
		QCIF,      
        CGA,       
		QVGA,      
        SIF,       
		WQVGA,     
		CIF,       
        W288P,     
        W368P,     
        S_448P,      
		VGA,       
        S_432P,      
        W432P,     
        S_4SIF,      
        W448P,     
		NTSC,		
        FW448P,    
        S_768x480P,  
		WVGA,      
		S_4CIF,      
		SVGA,      
        W544P,     
        W576P,     
		HD,        
		XGA,       
		WHD,       
		FULL_HD,   
        UXGA,      
		WFULL_HD,  
		NUMBER_OF_VIDEO_SIZE
	};

int GetWidthHeight(VideoSize size, int& width, int& height);


#endif  
