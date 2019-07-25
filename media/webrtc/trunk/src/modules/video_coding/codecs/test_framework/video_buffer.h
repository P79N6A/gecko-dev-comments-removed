









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_VIDEO_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_VIDEO_BUFFER_H_

#include "typedefs.h"
#include "video_image.h"

class TestVideoBuffer
{
public:
    TestVideoBuffer();

    virtual ~TestVideoBuffer();

    TestVideoBuffer(const TestVideoBuffer& rhs);

    



    void VerifyAndAllocate(unsigned int minimumSize);

    void UpdateLength(unsigned int newLength);

    void SwapBuffers(TestVideoBuffer& videoBuffer);

    void CopyBuffer(unsigned int length, const unsigned char* fromBuffer);

    void CopyBuffer(TestVideoBuffer& fromVideoBuffer);

    
    void CopyPointer(const TestVideoBuffer& fromVideoBuffer);

    void ClearPointer();

    int  SetOffset(unsigned int length);            

    void Free();                                    

    void SetTimeStamp(unsigned int timeStamp);      

    


    unsigned char* GetBuffer() const;

    


    unsigned int	GetSize() const;

    


    unsigned int	GetLength() const;

    


    unsigned int	GetTimeStamp() const;

    unsigned int	GetWidth() const;
    unsigned int	GetHeight() const;

    void            SetWidth(unsigned int width);
    void            SetHeight(unsigned int height);

private:
    TestVideoBuffer& operator=(const TestVideoBuffer& inBuffer);

private:
    void Set(unsigned char* buffer,unsigned int size,unsigned int length,unsigned int offset, unsigned int timeStamp);
    unsigned int GetStartOffset() const;

    unsigned char*		  _buffer;          
    unsigned int		  _bufferSize;      
    unsigned int		  _bufferLength;    
    unsigned int		  _startOffset;     
    unsigned int		  _timeStamp;       
    unsigned int          _width;
    unsigned int          _height;
};

class TestVideoEncodedBuffer: public TestVideoBuffer
{
public:
    TestVideoEncodedBuffer();
    ~TestVideoEncodedBuffer();

    void SetCaptureWidth(unsigned short width);
    void SetCaptureHeight(unsigned short height);
    unsigned short GetCaptureWidth();
    unsigned short GetCaptureHeight();

    webrtc::VideoFrameType GetFrameType();
    void SetFrameType(webrtc::VideoFrameType frametype);

    void Reset();

    void SetFrameRate(float frameRate);
    float GetFrameRate();

private:
    TestVideoEncodedBuffer& operator=(const TestVideoEncodedBuffer& inBuffer);

private:
    unsigned short			   _captureWidth;
    unsigned short			   _captureHeight;
    webrtc::VideoFrameType     _frameType;
    float                      _frameRate;
};

#endif 
