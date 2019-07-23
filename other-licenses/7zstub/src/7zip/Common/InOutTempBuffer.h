

#ifndef __IN_OUT_TEMP_BUFFER_H
#define __IN_OUT_TEMP_BUFFER_H

#include "../../Windows/FileIO.h"
#include "../../Windows/FileDir.h"
#include "../../Common/MyCom.h"

#include "../IStream.h"

class CInOutTempBuffer
{
  NWindows::NFile::NDirectory::CTempFile _tempFile;
  NWindows::NFile::NIO::COutFile _outFile;
  NWindows::NFile::NIO::CInFile _inFile;
  Byte *_buffer;
  UInt32 _bufferPosition;
  UInt32 _currentPositionInBuffer;
  CSysString _tmpFileName;
  bool _tmpFileCreated;

  UInt64 _fileSize;

  bool WriteToFile(const void *data, UInt32 size);
public:
  CInOutTempBuffer();
  ~CInOutTempBuffer();
  void Create();

  void InitWriting();
  bool Write(const void *data, UInt32 size);
  UInt64 GetDataSize() const { return _fileSize; }
  bool FlushWrite();
  bool InitReading();
  HRESULT WriteToStream(ISequentialOutStream *stream);
};

class CSequentialOutTempBufferImp: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CInOutTempBuffer *_buffer;
public:
  
  
  void Init(CInOutTempBuffer *buffer)  { _buffer = buffer; }
  

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

#endif
