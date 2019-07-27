






#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include "Http2Compression.h"
#include "Http2HuffmanIncoming.h"
#include "Http2HuffmanOutgoing.h"

extern PRThread *gSocketThread;

namespace mozilla {
namespace net {

static nsDeque *gStaticHeaders = nullptr;

void
Http2CompressionCleanup()
{
  
  delete gStaticHeaders;
  gStaticHeaders = nullptr;
}

static void
AddStaticElement(const nsCString &name, const nsCString &value)
{
  nvPair *pair = new nvPair(name, value);
  gStaticHeaders->Push(pair);
}

static void
AddStaticElement(const nsCString &name)
{
  AddStaticElement(name, EmptyCString());
}

static void
InitializeStaticHeaders()
{
  MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
  if (!gStaticHeaders) {
    gStaticHeaders = new nsDeque();
    AddStaticElement(NS_LITERAL_CSTRING(":authority"));
    AddStaticElement(NS_LITERAL_CSTRING(":method"), NS_LITERAL_CSTRING("GET"));
    AddStaticElement(NS_LITERAL_CSTRING(":method"), NS_LITERAL_CSTRING("POST"));
    AddStaticElement(NS_LITERAL_CSTRING(":path"), NS_LITERAL_CSTRING("/"));
    AddStaticElement(NS_LITERAL_CSTRING(":path"), NS_LITERAL_CSTRING("/index.html"));
    AddStaticElement(NS_LITERAL_CSTRING(":scheme"), NS_LITERAL_CSTRING("http"));
    AddStaticElement(NS_LITERAL_CSTRING(":scheme"), NS_LITERAL_CSTRING("https"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("200"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("204"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("206"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("304"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("400"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("404"));
    AddStaticElement(NS_LITERAL_CSTRING(":status"), NS_LITERAL_CSTRING("500"));
    AddStaticElement(NS_LITERAL_CSTRING("accept-charset"));
    AddStaticElement(NS_LITERAL_CSTRING("accept-encoding"), NS_LITERAL_CSTRING("gzip, deflate"));
    AddStaticElement(NS_LITERAL_CSTRING("accept-language"));
    AddStaticElement(NS_LITERAL_CSTRING("accept-ranges"));
    AddStaticElement(NS_LITERAL_CSTRING("accept"));
    AddStaticElement(NS_LITERAL_CSTRING("access-control-allow-origin"));
    AddStaticElement(NS_LITERAL_CSTRING("age"));
    AddStaticElement(NS_LITERAL_CSTRING("allow"));
    AddStaticElement(NS_LITERAL_CSTRING("authorization"));
    AddStaticElement(NS_LITERAL_CSTRING("cache-control"));
    AddStaticElement(NS_LITERAL_CSTRING("content-disposition"));
    AddStaticElement(NS_LITERAL_CSTRING("content-encoding"));
    AddStaticElement(NS_LITERAL_CSTRING("content-language"));
    AddStaticElement(NS_LITERAL_CSTRING("content-length"));
    AddStaticElement(NS_LITERAL_CSTRING("content-location"));
    AddStaticElement(NS_LITERAL_CSTRING("content-range"));
    AddStaticElement(NS_LITERAL_CSTRING("content-type"));
    AddStaticElement(NS_LITERAL_CSTRING("cookie"));
    AddStaticElement(NS_LITERAL_CSTRING("date"));
    AddStaticElement(NS_LITERAL_CSTRING("etag"));
    AddStaticElement(NS_LITERAL_CSTRING("expect"));
    AddStaticElement(NS_LITERAL_CSTRING("expires"));
    AddStaticElement(NS_LITERAL_CSTRING("from"));
    AddStaticElement(NS_LITERAL_CSTRING("host"));
    AddStaticElement(NS_LITERAL_CSTRING("if-match"));
    AddStaticElement(NS_LITERAL_CSTRING("if-modified-since"));
    AddStaticElement(NS_LITERAL_CSTRING("if-none-match"));
    AddStaticElement(NS_LITERAL_CSTRING("if-range"));
    AddStaticElement(NS_LITERAL_CSTRING("if-unmodified-since"));
    AddStaticElement(NS_LITERAL_CSTRING("last-modified"));
    AddStaticElement(NS_LITERAL_CSTRING("link"));
    AddStaticElement(NS_LITERAL_CSTRING("location"));
    AddStaticElement(NS_LITERAL_CSTRING("max-forwards"));
    AddStaticElement(NS_LITERAL_CSTRING("proxy-authenticate"));
    AddStaticElement(NS_LITERAL_CSTRING("proxy-authorization"));
    AddStaticElement(NS_LITERAL_CSTRING("range"));
    AddStaticElement(NS_LITERAL_CSTRING("referer"));
    AddStaticElement(NS_LITERAL_CSTRING("refresh"));
    AddStaticElement(NS_LITERAL_CSTRING("retry-after"));
    AddStaticElement(NS_LITERAL_CSTRING("server"));
    AddStaticElement(NS_LITERAL_CSTRING("set-cookie"));
    AddStaticElement(NS_LITERAL_CSTRING("strict-transport-security"));
    AddStaticElement(NS_LITERAL_CSTRING("transfer-encoding"));
    AddStaticElement(NS_LITERAL_CSTRING("user-agent"));
    AddStaticElement(NS_LITERAL_CSTRING("vary"));
    AddStaticElement(NS_LITERAL_CSTRING("via"));
    AddStaticElement(NS_LITERAL_CSTRING("www-authenticate"));
  }
}

nvFIFO::nvFIFO()
  : mByteCount(0)
  , mTable()
{
  InitializeStaticHeaders();
}

nvFIFO::~nvFIFO()
{
  Clear();
}

void
nvFIFO::AddElement(const nsCString &name, const nsCString &value)
{
  mByteCount += name.Length() + value.Length() + 32;
  nvPair *pair = new nvPair(name, value);
  mTable.PushFront(pair);
}

void
nvFIFO::AddElement(const nsCString &name)
{
  AddElement(name, EmptyCString());
}

void
nvFIFO::RemoveElement()
{
  nvPair *pair = static_cast<nvPair *>(mTable.Pop());
  if (pair) {
    mByteCount -= pair->Size();
    delete pair;
  }
}

uint32_t
nvFIFO::ByteCount() const
{
  return mByteCount;
}

uint32_t
nvFIFO::Length() const
{
  return mTable.GetSize() + gStaticHeaders->GetSize();
}

uint32_t
nvFIFO::VariableLength() const
{
  return mTable.GetSize();
}

uint32_t
nvFIFO::StaticLength() const
{
  return gStaticHeaders->GetSize();
}

void
nvFIFO::Clear()
{
  mByteCount = 0;
  while (mTable.GetSize())
    delete static_cast<nvPair *>(mTable.Pop());
}

const nvPair *
nvFIFO::operator[] (int32_t index) const
{
  
  
  if (index >= (mTable.GetSize() + gStaticHeaders->GetSize())) {
    MOZ_ASSERT(false);
    NS_WARNING("nvFIFO Table Out of Range");
    return nullptr;
  }
  if (index >= gStaticHeaders->GetSize()) {
    return static_cast<nvPair *>(mTable.ObjectAt(index - gStaticHeaders->GetSize()));
  }
  return static_cast<nvPair *>(gStaticHeaders->ObjectAt(index));
}

Http2BaseCompressor::Http2BaseCompressor()
  : mOutput(nullptr)
  , mMaxBuffer(kDefaultMaxBuffer)
{
}

void
Http2BaseCompressor::ClearHeaderTable()
{
  mHeaderTable.Clear();
}

void
Http2BaseCompressor::MakeRoom(uint32_t amount, const char *direction)
{
  
  while (mHeaderTable.VariableLength() && ((mHeaderTable.ByteCount() + amount) > mMaxBuffer)) {
    
    uint32_t index = mHeaderTable.Length() - 1;
    LOG(("HTTP %s header table index %u %s %s removed for size.\n",
         direction, index, mHeaderTable[index]->mName.get(),
         mHeaderTable[index]->mValue.get()));
    mHeaderTable.RemoveElement();
  }
}

void
Http2BaseCompressor::DumpState()
{
  LOG(("Header Table"));
  uint32_t i;
  uint32_t length = mHeaderTable.Length();
  uint32_t staticLength = mHeaderTable.StaticLength();
  
  for (i = 0; i < length; ++i) {
    const nvPair *pair = mHeaderTable[i];
    
    LOG(("%sindex %u: %s %s", i < staticLength ? "static " : "", i,
         pair->mName.get(), pair->mValue.get()));
  }
}

nsresult
Http2Decompressor::DecodeHeaderBlock(const uint8_t *data, uint32_t datalen,
                                     nsACString &output, bool isPush)
{
  mOffset = 0;
  mData = data;
  mDataLen = datalen;
  mOutput = &output;
  mOutput->Truncate();
  mHeaderStatus.Truncate();
  mHeaderHost.Truncate();
  mHeaderScheme.Truncate();
  mHeaderPath.Truncate();
  mHeaderMethod.Truncate();
  mSeenNonColonHeader = false;
  mIsPush = isPush;

  nsresult rv = NS_OK;
  while (NS_SUCCEEDED(rv) && (mOffset < datalen)) {
    if (mData[mOffset] & 0x80) {
      rv = DoIndexed();
      LOG(("Decompressor state after indexed"));
    } else if (mData[mOffset] & 0x40) {
      rv = DoLiteralWithIncremental();
      LOG(("Decompressor state after literal with incremental"));
    } else if (mData[mOffset] & 0x20) {
      rv = DoContextUpdate();
      LOG(("Decompressor state after context update"));
    } else if (mData[mOffset] & 0x10) {
      rv = DoLiteralNeverIndexed();
      LOG(("Decompressor state after literal never index"));
    } else {
      rv = DoLiteralWithoutIndex();
      LOG(("Decompressor state after literal without index"));
    }
    DumpState();
  }

  return rv;
}

nsresult
Http2Decompressor::DecodeInteger(uint32_t prefixLen, uint32_t &accum)
{
  accum = 0;

  if (prefixLen) {
    uint32_t mask = (1 << prefixLen) - 1;

    accum = mData[mOffset] & mask;
    ++mOffset;

    if (accum != mask) {
      
      return NS_OK;
    }
  }

  uint32_t factor = 1; 

  
  

  if (mOffset >= mDataLen) {
    NS_WARNING("Ran out of data to decode integer");
    return NS_ERROR_ILLEGAL_VALUE;
  }
  bool chainBit = mData[mOffset] & 0x80;
  accum += (mData[mOffset] & 0x7f) * factor;

  ++mOffset;
  factor = factor * 128;

  while (chainBit) {
    
    if (accum >= 0x800000) {
      NS_WARNING("Decoding integer >= 0x800000");
      return NS_ERROR_ILLEGAL_VALUE;
    }

    if (mOffset >= mDataLen) {
      NS_WARNING("Ran out of data to decode integer");
      return NS_ERROR_ILLEGAL_VALUE;
    }
    chainBit = mData[mOffset] & 0x80;
    accum += (mData[mOffset] & 0x7f) * factor;
    ++mOffset;
    factor = factor * 128;
  }
  return NS_OK;
}

nsresult
Http2Decompressor::OutputHeader(const nsACString &name, const nsACString &value)
{
    
  if (!mIsPush &&
      (name.EqualsLiteral("connection") ||
       name.EqualsLiteral("host") ||
       name.EqualsLiteral("keep-alive") ||
       name.EqualsLiteral("proxy-connection") ||
       name.EqualsLiteral("te") ||
       name.EqualsLiteral("transfer-encoding") ||
       name.EqualsLiteral("upgrade") ||
       name.Equals(("accept-encoding")))) {
    nsCString toLog(name);
    LOG(("HTTP Decompressor illegal response header found, not gatewaying: %s",
         toLog.get()));
    return NS_OK;
  }

  
  for (const char *cPtr = name.BeginReading();
       cPtr && cPtr < name.EndReading();
       ++cPtr) {
    if (*cPtr <= 'Z' && *cPtr >= 'A') {
      nsCString toLog(name);
      LOG(("HTTP Decompressor upper case response header found. [%s]\n",
           toLog.get()));
      return NS_ERROR_ILLEGAL_VALUE;
    }
  }

  
  
  for (const char *cPtr = value.BeginReading();
       cPtr && cPtr < value.EndReading();
       ++cPtr) {
    if (*cPtr == '\r' || *cPtr== '\n') {
      char *wPtr = const_cast<char *>(cPtr);
      *wPtr = ' ';
    }
  }

  
  if (name.EqualsLiteral(":status")) {
    nsAutoCString status(NS_LITERAL_CSTRING("HTTP/2.0 "));
    status.Append(value);
    status.AppendLiteral("\r\n");
    mOutput->Insert(status, 0);
    mHeaderStatus = value;
  } else if (name.EqualsLiteral(":authority")) {
    mHeaderHost = value;
  } else if (name.EqualsLiteral(":scheme")) {
    mHeaderScheme = value;
  } else if (name.EqualsLiteral(":path")) {
    mHeaderPath = value;
  } else if (name.EqualsLiteral(":method")) {
    mHeaderMethod = value;
  }

  
  bool isColonHeader = false;
  for (const char *cPtr = name.BeginReading();
       cPtr && cPtr < name.EndReading();
       ++cPtr) {
    if (*cPtr == ':') {
      isColonHeader = true;
      break;
    } else if (*cPtr != ' ' && *cPtr != '\t') {
      isColonHeader = false;
      break;
    }
  }
  if(isColonHeader) {
    if (mSeenNonColonHeader) {
      LOG(("HTTP Decompressor found illegal : header %s", name.BeginReading()));
      return NS_ERROR_ILLEGAL_VALUE;
    }
    LOG(("HTTP Decompressor not gatewaying %s into http/1",
         name.BeginReading()));
    return NS_OK;
  }

  LOG(("Http2Decompressor::OutputHeader %s %s", name.BeginReading(),
       value.BeginReading()));
  mSeenNonColonHeader = true;
  mOutput->Append(name);
  mOutput->AppendLiteral(": ");
  mOutput->Append(value);
  mOutput->AppendLiteral("\r\n");
  return NS_OK;
}

nsresult
Http2Decompressor::OutputHeader(uint32_t index)
{
  
  
  if (mHeaderTable.Length() <= index) {
    LOG(("Http2Decompressor::OutputHeader index too large %u", index));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  return OutputHeader(mHeaderTable[index]->mName,
                      mHeaderTable[index]->mValue);
}

nsresult
Http2Decompressor::CopyHeaderString(uint32_t index, nsACString &name)
{
  
  
  if (mHeaderTable.Length() <= index)
    return NS_ERROR_ILLEGAL_VALUE;

  name = mHeaderTable[index]->mName;
  return NS_OK;
}

nsresult
Http2Decompressor::CopyStringFromInput(uint32_t bytes, nsACString &val)
{
  if (mOffset + bytes > mDataLen)
    return NS_ERROR_ILLEGAL_VALUE;

  val.Assign(reinterpret_cast<const char *>(mData) + mOffset, bytes);
  mOffset += bytes;
  return NS_OK;
}

nsresult
Http2Decompressor::DecodeFinalHuffmanCharacter(HuffmanIncomingTable *table,
                                               uint8_t &c, uint8_t &bitsLeft)
{
  uint8_t mask = (1 << bitsLeft) - 1;
  uint8_t idx = mData[mOffset - 1] & mask;
  idx <<= (8 - bitsLeft);
  
  
  

  HuffmanIncomingEntry *entry = &(table->mEntries[idx]);

  if (entry->mPtr) {
    
    LOG(("DecodeFinalHuffmanCharacter trying to chain when we're out of bits"));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (bitsLeft < entry->mPrefixLen) {
    
    
    LOG(("DecodeFinalHuffmanCharacter does't have enough bits to match"));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  if (entry->mValue == 256) {
    
    LOG(("DecodeFinalHuffmanCharacter actually decoded an EOS"));
    return NS_ERROR_ILLEGAL_VALUE;
  }
  c = static_cast<uint8_t>(entry->mValue & 0xFF);
  bitsLeft -= entry->mPrefixLen;

  return NS_OK;
}

uint8_t
Http2Decompressor::ExtractByte(uint8_t bitsLeft, uint32_t &bytesConsumed)
{
  uint8_t rv;

  if (bitsLeft) {
    
    
    uint8_t mask = (1 << bitsLeft) - 1;
    rv = (mData[mOffset - 1] & mask) << (8 - bitsLeft);
    rv |= (mData[mOffset] & ~mask) >> bitsLeft;
  } else {
    rv = mData[mOffset];
  }

  
  
  
  ++mOffset;
  ++bytesConsumed;

  return rv;
}

nsresult
Http2Decompressor::DecodeHuffmanCharacter(HuffmanIncomingTable *table,
                                          uint8_t &c, uint32_t &bytesConsumed,
                                          uint8_t &bitsLeft)
{
  uint8_t idx = ExtractByte(bitsLeft, bytesConsumed);
  HuffmanIncomingEntry *entry = &(table->mEntries[idx]);

  if (entry->mPtr) {
    if (bytesConsumed >= mDataLen) {
      if (!bitsLeft || (bytesConsumed > mDataLen)) {
        
        
        LOG(("DecodeHuffmanCharacter all out of bits to consume, can't chain"));
        return NS_ERROR_ILLEGAL_VALUE;
      }

      
      return DecodeFinalHuffmanCharacter(entry->mPtr, c, bitsLeft);
    }

    
    return DecodeHuffmanCharacter(entry->mPtr, c, bytesConsumed, bitsLeft);
  }

  if (entry->mValue == 256) {
    LOG(("DecodeHuffmanCharacter found an actual EOS"));
    return NS_ERROR_ILLEGAL_VALUE;
  }
  c = static_cast<uint8_t>(entry->mValue & 0xFF);

  
  
  if (entry->mPrefixLen <= bitsLeft) {
    bitsLeft -= entry->mPrefixLen;
    --mOffset;
    --bytesConsumed;
  } else {
    bitsLeft = 8 - (entry->mPrefixLen - bitsLeft);
  }
  MOZ_ASSERT(bitsLeft < 8);

  return NS_OK;
}

nsresult
Http2Decompressor::CopyHuffmanStringFromInput(uint32_t bytes, nsACString &val)
{
  if (mOffset + bytes > mDataLen) {
    LOG(("CopyHuffmanStringFromInput not enough data"));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t bytesRead = 0;
  uint8_t bitsLeft = 0;
  nsAutoCString buf;
  nsresult rv;
  uint8_t c;

  while (bytesRead < bytes) {
    uint32_t bytesConsumed = 0;
    rv = DecodeHuffmanCharacter(&HuffmanIncomingRoot, c, bytesConsumed,
                                bitsLeft);
    if (NS_FAILED(rv)) {
      LOG(("CopyHuffmanStringFromInput failed to decode a character"));
      return rv;
    }

    bytesRead += bytesConsumed;
    buf.Append(c);
  }

  if (bytesRead > bytes) {
    LOG(("CopyHuffmanStringFromInput read more bytes than was allowed!"));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (bitsLeft) {
    
    
    
    rv = DecodeFinalHuffmanCharacter(&HuffmanIncomingRoot, c, bitsLeft);
    if (NS_SUCCEEDED(rv)) {
      buf.Append(c);
    }
  }

  if (bitsLeft > 7) {
    LOG(("CopyHuffmanStringFromInput more than 7 bits of padding"));
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (bitsLeft) {
    
    
    uint8_t mask = (1 << bitsLeft) - 1;
    uint8_t bits = mData[mOffset - 1] & mask;
    if (bits != mask) {
      LOG(("CopyHuffmanStringFromInput ran out of data but found possible "
           "non-EOS symbol"));
      return NS_ERROR_ILLEGAL_VALUE;
    }
  }

  val = buf;
  LOG(("CopyHuffmanStringFromInput decoded a full string!"));
  return NS_OK;
}

nsresult
Http2Decompressor::DoIndexed()
{
  
  MOZ_ASSERT(mData[mOffset] & 0x80);

  

  uint32_t index;
  nsresult rv = DecodeInteger(7, index);
  if (NS_FAILED(rv))
    return rv;

  LOG(("HTTP decompressor indexed entry %u\n", index));

  if (index == 0) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  index--; 

  return OutputHeader(index);
}

nsresult
Http2Decompressor::DoLiteralInternal(nsACString &name, nsACString &value,
                                     uint32_t namePrefixLen)
{
  
  MOZ_ASSERT(((mData[mOffset] & 0xF0) == 0x00) ||  
             ((mData[mOffset] & 0xF0) == 0x10) ||  
             ((mData[mOffset] & 0xC0) == 0x40));   

  
  uint32_t index;
  nsresult rv = DecodeInteger(namePrefixLen, index);
  if (NS_FAILED(rv))
    return rv;

  bool isHuffmanEncoded;

  if (!index) {
    
    uint32_t nameLen;
    isHuffmanEncoded = mData[mOffset] & (1 << 7);
    rv = DecodeInteger(7, nameLen);
    if (NS_SUCCEEDED(rv)) {
      if (isHuffmanEncoded) {
        rv = CopyHuffmanStringFromInput(nameLen, name);
      } else {
        rv = CopyStringFromInput(nameLen, name);
      }
    }
    LOG(("Http2Decompressor::DoLiteralInternal literal name %s",
         name.BeginReading()));
  } else {
    
    
    rv = CopyHeaderString(index - 1, name);
    LOG(("Http2Decompressor::DoLiteralInternal indexed name %d %s",
         index, name.BeginReading()));
  }
  if (NS_FAILED(rv))
    return rv;

  
  uint32_t valueLen;
  isHuffmanEncoded = mData[mOffset] & (1 << 7);
  rv = DecodeInteger(7, valueLen);
  if (NS_SUCCEEDED(rv)) {
    if (isHuffmanEncoded) {
      rv = CopyHuffmanStringFromInput(valueLen, value);
    } else {
      rv = CopyStringFromInput(valueLen, value);
    }
  }
  if (NS_FAILED(rv))
    return rv;
  LOG(("Http2Decompressor::DoLiteralInternal value %s", value.BeginReading()));
  return NS_OK;
}

nsresult
Http2Decompressor::DoLiteralWithoutIndex()
{
  
  MOZ_ASSERT((mData[mOffset] & 0xF0) == 0x00);

  nsAutoCString name, value;
  nsresult rv = DoLiteralInternal(name, value, 4);

  LOG(("HTTP decompressor literal without index %s %s\n",
       name.get(), value.get()));

  if (NS_SUCCEEDED(rv))
    rv = OutputHeader(name, value);
  return rv;
}

nsresult
Http2Decompressor::DoLiteralWithIncremental()
{
  
  MOZ_ASSERT((mData[mOffset] & 0xC0) == 0x40);

  nsAutoCString name, value;
  nsresult rv = DoLiteralInternal(name, value, 6);
  if (NS_SUCCEEDED(rv))
    rv = OutputHeader(name, value);
  if (NS_FAILED(rv))
    return rv;

  uint32_t room = nvPair(name, value).Size();
  if (room > mMaxBuffer) {
    ClearHeaderTable();
    LOG(("HTTP decompressor literal with index not inserted due to size %u %s %s\n",
         room, name.get(), value.get()));
    LOG(("Decompressor state after ClearHeaderTable"));
    DumpState();
    return NS_OK;
  }

  MakeRoom(room, "decompressor");

  
  mHeaderTable.AddElement(name, value);

  LOG(("HTTP decompressor literal with index 0 %s %s\n",
       name.get(), value.get()));

  return NS_OK;
}

nsresult
Http2Decompressor::DoLiteralNeverIndexed()
{
  
  MOZ_ASSERT((mData[mOffset] & 0xF0) == 0x10);

  nsAutoCString name, value;
  nsresult rv = DoLiteralInternal(name, value, 4);

  LOG(("HTTP decompressor literal never indexed %s %s\n",
       name.get(), value.get()));

  if (NS_SUCCEEDED(rv))
    rv = OutputHeader(name, value);
  return rv;
}

nsresult
Http2Decompressor::DoContextUpdate()
{
  
  MOZ_ASSERT((mData[mOffset] & 0xE0) == 0x20);

  
  uint32_t newMaxSize;
  nsresult rv = DecodeInteger(5, newMaxSize);
  LOG(("Http2Decompressor::DoContextUpdate new maximum size %u", newMaxSize));
  if (NS_FAILED(rv))
    return rv;
  return mCompressor->SetMaxBufferSizeInternal(newMaxSize);
}



nsresult
Http2Compressor::EncodeHeaderBlock(const nsCString &nvInput,
                                   const nsACString &method, const nsACString &path,
                                   const nsACString &host, const nsACString &scheme,
                                   bool connectForm, nsACString &output)
{
  mOutput = &output;
  output.SetCapacity(1024);
  output.Truncate();
  mParsedContentLength = -1;

  
  if (mBufferSizeChangeWaiting) {
    if (mLowestBufferSizeWaiting < mMaxBufferSetting) {
      EncodeTableSizeChange(mLowestBufferSizeWaiting);
    }
    EncodeTableSizeChange(mMaxBufferSetting);
    mBufferSizeChangeWaiting = false;
  }

  
  if (!connectForm) {
    ProcessHeader(nvPair(NS_LITERAL_CSTRING(":method"), method), false, false);
    ProcessHeader(nvPair(NS_LITERAL_CSTRING(":path"), path), true, false);
    ProcessHeader(nvPair(NS_LITERAL_CSTRING(":authority"), host), false, false);
    ProcessHeader(nvPair(NS_LITERAL_CSTRING(":scheme"), scheme), false, false);
  } else {
    ProcessHeader(nvPair(NS_LITERAL_CSTRING(":method"), method), false, false);
    ProcessHeader(nvPair(NS_LITERAL_CSTRING(":authority"), host), false, false);
  }

  
  const char *beginBuffer = nvInput.BeginReading();

  
  int32_t crlfIndex = nvInput.Find("\r\n");
  while (true) {
    int32_t startIndex = crlfIndex + 2;

    crlfIndex = nvInput.Find("\r\n", false, startIndex);
    if (crlfIndex == -1)
      break;

    int32_t colonIndex = nvInput.Find(":", false, startIndex,
                                      crlfIndex - startIndex);
    if (colonIndex == -1)
      break;

    nsDependentCSubstring name = Substring(beginBuffer + startIndex,
                                           beginBuffer + colonIndex);
    
    ToLowerCase(name);

    
    if (name.EqualsLiteral("connection") ||
        name.EqualsLiteral("host") ||
        name.EqualsLiteral("keep-alive") ||
        name.EqualsLiteral("proxy-connection") ||
        name.EqualsLiteral("te") ||
        name.EqualsLiteral("transfer-encoding") ||
        name.EqualsLiteral("upgrade")) {
      continue;
    }

    
    
    bool isColonHeader = false;
    for (const char *cPtr = name.BeginReading();
         cPtr && cPtr < name.EndReading();
         ++cPtr) {
      if (*cPtr == ':') {
        isColonHeader = true;
        break;
      } else if (*cPtr != ' ' && *cPtr != '\t') {
        isColonHeader = false;
        break;
      }
    }
    if(isColonHeader) {
      continue;
    }

    int32_t valueIndex = colonIndex + 1;

    
    
    
    if (name.EqualsLiteral("expect")) {
      const char *continueHeader =
        nsHttp::FindToken(beginBuffer + valueIndex, "100-continue",
                          HTTP_HEADER_VALUE_SEPS);
      if (continueHeader) {
        char *writableVal = const_cast<char *>(continueHeader);
        memset(writableVal, 0, 12);
        writableVal += 12;
        
        while ((*writableVal == ' ') || (*writableVal == '\t') ||
               (*writableVal == ',')) {
          *writableVal = ' ';
          ++writableVal;
        }
      }
    }

    while (valueIndex < crlfIndex && beginBuffer[valueIndex] == ' ')
      ++valueIndex;

    nsDependentCSubstring value = Substring(beginBuffer + valueIndex,
                                            beginBuffer + crlfIndex);

    if (name.EqualsLiteral("content-length")) {
      int64_t len;
      nsCString tmp(value);
      if (nsHttp::ParseInt64(tmp.get(), nullptr, &len))
        mParsedContentLength = len;
    }

    if (name.EqualsLiteral("cookie")) {
      
      bool haveMoreCookies = true;
      int32_t nextCookie = valueIndex;
      while (haveMoreCookies) {
        int32_t semiSpaceIndex = nvInput.Find("; ", false, nextCookie,
                                              crlfIndex - nextCookie);
        if (semiSpaceIndex == -1) {
          haveMoreCookies = false;
          semiSpaceIndex = crlfIndex;
        }
        nsDependentCSubstring cookie = Substring(beginBuffer + nextCookie,
                                                 beginBuffer + semiSpaceIndex);
        
        ProcessHeader(nvPair(name, cookie), false, name.Length() < 20);
        nextCookie = semiSpaceIndex + 2;
      }
    } else {
      
      ProcessHeader(nvPair(name, value), false,
                    name.EqualsLiteral("authorization"));
    }
  }

  mOutput = nullptr;
  LOG(("Compressor state after EncodeHeaderBlock"));
  DumpState();
  return NS_OK;
}

void
Http2Compressor::DoOutput(Http2Compressor::outputCode code,
                          const class nvPair *pair, uint32_t index)
{
  
  
  
  uint32_t offset = mOutput->Length();
  uint8_t *startByte;

  switch (code) {
  case kNeverIndexedLiteral:
    LOG(("HTTP compressor %p neverindex literal with name reference %u %s %s\n",
         this, index, pair->mName.get(), pair->mValue.get()));

    
    
    EncodeInteger(4, index); 
    startByte = reinterpret_cast<unsigned char *>(mOutput->BeginWriting()) + offset;
    *startByte = (*startByte & 0x0f) | 0x10;

    if (!index) {
      HuffmanAppend(pair->mName);
    }

    HuffmanAppend(pair->mValue);
    break;

  case kPlainLiteral:
    LOG(("HTTP compressor %p noindex literal with name reference %u %s %s\n",
         this, index, pair->mName.get(), pair->mValue.get()));

    
    
    EncodeInteger(4, index); 
    startByte = reinterpret_cast<unsigned char *>(mOutput->BeginWriting()) + offset;
    *startByte = *startByte & 0x0f;

    if (!index) {
      HuffmanAppend(pair->mName);
    }

    HuffmanAppend(pair->mValue);
    break;

  case kIndexedLiteral:
    LOG(("HTTP compressor %p literal with name reference %u %s %s\n",
         this, index, pair->mName.get(), pair->mValue.get()));

    
    
    EncodeInteger(6, index); 
    startByte = reinterpret_cast<unsigned char *>(mOutput->BeginWriting()) + offset;
    *startByte = (*startByte & 0x3f) | 0x40;

    if (!index) {
      HuffmanAppend(pair->mName);
    }

    HuffmanAppend(pair->mValue);
    break;

  case kIndex:
    LOG(("HTTP compressor %p index %u %s %s\n",
         this, index, pair->mName.get(), pair->mValue.get()));
    
    
    
    EncodeInteger(7, index + 1);
    startByte = reinterpret_cast<unsigned char *>(mOutput->BeginWriting()) + offset;
    *startByte = *startByte | 0x80; 
    break;

  }
}


void
Http2Compressor::EncodeInteger(uint32_t prefixLen, uint32_t val)
{
  uint32_t mask = (1 << prefixLen) - 1;
  uint8_t tmp;

  if (val < mask) {
    
    tmp = val;
    mOutput->Append(reinterpret_cast<char *>(&tmp), 1);
    return;
  }

  if (mask) {
    val -= mask;
    tmp = mask;
    mOutput->Append(reinterpret_cast<char *>(&tmp), 1);
  }

  uint32_t q, r;
  do {
    q = val / 128;
    r = val % 128;
    tmp = r;
    if (q)
      tmp |= 0x80; 
    val = q;
    mOutput->Append(reinterpret_cast<char *>(&tmp), 1);
  } while (q);
}

void
Http2Compressor::HuffmanAppend(const nsCString &value)
{
  nsAutoCString buf;
  uint8_t bitsLeft = 8;
  uint32_t length = value.Length();
  uint32_t offset;
  uint8_t *startByte;

  for (uint32_t i = 0; i < length; ++i) {
    uint8_t idx = static_cast<uint8_t>(value[i]);
    uint8_t huffLength = HuffmanOutgoing[idx].mLength;
    uint32_t huffValue = HuffmanOutgoing[idx].mValue;

    if (bitsLeft < 8) {
      
      
      uint32_t val;
      if (huffLength >= bitsLeft) {
        val = huffValue & ~((1 << (huffLength - bitsLeft)) - 1);
        val >>= (huffLength - bitsLeft);
      } else {
        val = huffValue << (bitsLeft - huffLength);
      }
      val &= ((1 << bitsLeft) - 1);
      offset = buf.Length() - 1;
      startByte = reinterpret_cast<unsigned char *>(buf.BeginWriting()) + offset;
      *startByte = *startByte | static_cast<uint8_t>(val & 0xFF);
      if (huffLength >= bitsLeft) {
        huffLength -= bitsLeft;
        bitsLeft = 8;
      } else {
        bitsLeft -= huffLength;
        huffLength = 0;
      }
    }

    while (huffLength >= 8) {
      uint32_t mask = ~((1 << (huffLength - 8)) - 1);
      uint8_t val = ((huffValue & mask) >> (huffLength - 8)) & 0xFF;
      buf.Append(reinterpret_cast<char *>(&val), 1);
      huffLength -= 8;
    }

    if (huffLength) {
      
      bitsLeft = 8 - huffLength;
      uint8_t val = (huffValue & ((1 << huffLength) - 1)) << bitsLeft;
      buf.Append(reinterpret_cast<char *>(&val), 1);
    }
  }

  if (bitsLeft != 8) {
    
    
    uint8_t val = (1 << bitsLeft) - 1;
    offset = buf.Length() - 1;
    startByte = reinterpret_cast<unsigned char *>(buf.BeginWriting()) + offset;
    *startByte = *startByte | val;
  }

  
  uint32_t bufLength = buf.Length();
  offset = mOutput->Length();
  EncodeInteger(7, bufLength);
  startByte = reinterpret_cast<unsigned char *>(mOutput->BeginWriting()) + offset;
  *startByte = *startByte | 0x80;

  
  mOutput->Append(buf);
  LOG(("Http2Compressor::HuffmanAppend %p encoded %d byte original on %d "
       "bytes.\n", this, length, bufLength));
}

void
Http2Compressor::ProcessHeader(const nvPair inputPair, bool noLocalIndex,
                               bool neverIndex)
{
  uint32_t newSize = inputPair.Size();
  uint32_t headerTableSize = mHeaderTable.Length();
  uint32_t matchedIndex;
  uint32_t nameReference = 0;
  bool match = false;

  LOG(("Http2Compressor::ProcessHeader %s %s", inputPair.mName.get(),
       inputPair.mValue.get()));

  
  for (uint32_t index = 0; index < headerTableSize; ++index) {
    if (mHeaderTable[index]->mName.Equals(inputPair.mName)) {
      
      nameReference = index + 1;
      if (mHeaderTable[index]->mValue.Equals(inputPair.mValue)) {
        match = true;
        matchedIndex = index;
        break;
      }
    }
  }

  
  if (!match || noLocalIndex || neverIndex) {
    if (neverIndex) {
      DoOutput(kNeverIndexedLiteral, &inputPair, nameReference);
      LOG(("Compressor state after literal never index"));
      DumpState();
      return;
    }

    if (noLocalIndex || (newSize > (mMaxBuffer / 2)) || (mMaxBuffer < 128)) {
      DoOutput(kPlainLiteral, &inputPair, nameReference);
      LOG(("Compressor state after literal without index"));
      DumpState();
      return;
    }

    
    
    MakeRoom(newSize, "compressor");
    DoOutput(kIndexedLiteral, &inputPair, nameReference);

    mHeaderTable.AddElement(inputPair.mName, inputPair.mValue);
    LOG(("HTTP compressor %p new literal placed at index 0\n",
         this));
    LOG(("Compressor state after literal with index"));
    DumpState();
    return;
  }

  
  DoOutput(kIndex, &inputPair, matchedIndex);

  LOG(("Compressor state after index"));
  DumpState();
  return;
}

void
Http2Compressor::EncodeTableSizeChange(uint32_t newMaxSize)
{
  uint32_t offset = mOutput->Length();
  EncodeInteger(5, newMaxSize);
  uint8_t *startByte = reinterpret_cast<uint8_t *>(mOutput->BeginWriting()) + offset;
  *startByte = *startByte | 0x20;
}

void
Http2Compressor::SetMaxBufferSize(uint32_t maxBufferSize)
{
  mMaxBufferSetting = maxBufferSize;
  SetMaxBufferSizeInternal(maxBufferSize);
  if (!mBufferSizeChangeWaiting) {
    mBufferSizeChangeWaiting = true;
    mLowestBufferSizeWaiting = maxBufferSize;
  } else if (maxBufferSize < mLowestBufferSizeWaiting) {
    mLowestBufferSizeWaiting = maxBufferSize;
  }
}

nsresult
Http2Compressor::SetMaxBufferSizeInternal(uint32_t maxBufferSize)
{
  if (maxBufferSize > mMaxBufferSetting) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t removedCount = 0;

  LOG(("Http2Compressor::SetMaxBufferSizeInternal %u called", maxBufferSize));

  while (mHeaderTable.VariableLength() && (mHeaderTable.ByteCount() > maxBufferSize)) {
    mHeaderTable.RemoveElement();
    ++removedCount;
  }

  mMaxBuffer = maxBufferSize;

  return NS_OK;
}

} 
} 
