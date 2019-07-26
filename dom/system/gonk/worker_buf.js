



const INT32_MAX   = 2147483647;
const UINT8_SIZE  = 1;
const UINT16_SIZE = 2;
const UINT32_SIZE = 4;
const PARCEL_SIZE_SIZE = UINT32_SIZE;














let mIncomingBufferLength = 1024;
let mOutgoingBufferLength = 1024;
let mIncomingBuffer, mOutgoingBuffer, mIncomingBytes, mOutgoingBytes,
    mIncomingWriteIndex, mIncomingReadIndex, mOutgoingIndex, mReadIncoming,
    mReadAvailable, mCurrentParcelSize, mToken, mTokenRequestMap,
    mLasSolicitedToken, mOutgoingBufferCalSizeQueue, mOutputStream;

function init() {
  mIncomingBuffer = new ArrayBuffer(mIncomingBufferLength);
  mOutgoingBuffer = new ArrayBuffer(mOutgoingBufferLength);

  mIncomingBytes = new Uint8Array(mIncomingBuffer);
  mOutgoingBytes = new Uint8Array(mOutgoingBuffer);

  
  mIncomingWriteIndex = 0;
  mIncomingReadIndex = 0;

  
  mOutgoingIndex = PARCEL_SIZE_SIZE;

  
  mReadIncoming = 0;

  
  mReadAvailable = 0;

  
  
  mCurrentParcelSize = 0;

  
  mToken = 1;

  
  
  mTokenRequestMap = {};

  
  mLasSolicitedToken = 0;

  
  mOutgoingBufferCalSizeQueue = [];
}













function startCalOutgoingSize(writeFunction) {
  let sizeInfo = {index: mOutgoingIndex,
                  write: writeFunction};

  
  writeFunction.call(0);

  
  sizeInfo.size = mOutgoingIndex - sizeInfo.index;

  
  mOutgoingBufferCalSizeQueue.push(sizeInfo);
}




function stopCalOutgoingSize() {
  let sizeInfo = mOutgoingBufferCalSizeQueue.pop();

  
  let currentOutgoingIndex = mOutgoingIndex;
  
  let writeSize = mOutgoingIndex - sizeInfo.index - sizeInfo.size;

  
  
  mOutgoingIndex = sizeInfo.index;
  sizeInfo.write(writeSize);

  
  mOutgoingIndex = currentOutgoingIndex;
}








function growIncomingBuffer(min_size) {
  if (DEBUG) {
    debug("Current buffer of " + mIncomingBufferLength +
          " can't handle incoming " + min_size + " bytes.");
  }
  let oldBytes = mIncomingBytes;
  mIncomingBufferLength =
    2 << Math.floor(Math.log(min_size)/Math.log(2));
  if (DEBUG) debug("New incoming buffer size: " + mIncomingBufferLength);
  mIncomingBuffer = new ArrayBuffer(mIncomingBufferLength);
  mIncomingBytes = new Uint8Array(mIncomingBuffer);
  if (mIncomingReadIndex <= mIncomingWriteIndex) {
    
    
    
    mIncomingBytes.set(oldBytes, 0);
  } else {
    
    
    
    
    let head = oldBytes.subarray(mIncomingReadIndex);
    let tail = oldBytes.subarray(0, mIncomingReadIndex);
    mIncomingBytes.set(head, 0);
    mIncomingBytes.set(tail, head.length);
    mIncomingReadIndex = 0;
    mIncomingWriteIndex += head.length;
  }
  if (DEBUG) {
    debug("New incoming buffer size is " + mIncomingBufferLength);
  }
}








function growOutgoingBuffer(min_size) {
  if (DEBUG) {
    debug("Current buffer of " + mOutgoingBufferLength +
          " is too small.");
  }
  let oldBytes = mOutgoingBytes;
  mOutgoingBufferLength =
    2 << Math.floor(Math.log(min_size)/Math.log(2));
  mOutgoingBuffer = new ArrayBuffer(mOutgoingBufferLength);
  mOutgoingBytes = new Uint8Array(mOutgoingBuffer);
  mOutgoingBytes.set(oldBytes, 0);
  if (DEBUG) {
    debug("New outgoing buffer size is " + mOutgoingBufferLength);
  }
}














function ensureIncomingAvailable(index) {
  if (index >= mCurrentParcelSize) {
    throw new Error("Trying to read data beyond the parcel end!");
  } else if (index < 0) {
    throw new Error("Trying to read data before the parcel begin!");
  }
}







function seekIncoming(offset) {
  
  let cur = mCurrentParcelSize - mReadAvailable;

  let newIndex = cur + offset;
  ensureIncomingAvailable(newIndex);

  
  
  
  
  
  mReadAvailable = mCurrentParcelSize - newIndex;

  
  if (mIncomingReadIndex < cur) {
    
    newIndex += mIncomingBufferLength;
  }
  newIndex += (mIncomingReadIndex - cur);
  newIndex %= mIncomingBufferLength;
  mIncomingReadIndex = newIndex;
}

function readUint8Unchecked() {
  let value = mIncomingBytes[mIncomingReadIndex];
  mIncomingReadIndex = (mIncomingReadIndex + 1) %
                           mIncomingBufferLength;
  return value;
}

function readUint8() {
  
  let cur = mCurrentParcelSize - mReadAvailable;
  ensureIncomingAvailable(cur);

  mReadAvailable--;
  return readUint8Unchecked();
}

function readUint8Array(length) {
  
  let last = mCurrentParcelSize - mReadAvailable;
  last += (length - 1);
  ensureIncomingAvailable(last);

  let array = new Uint8Array(length);
  for (let i = 0; i < length; i++) {
    array[i] = readUint8Unchecked();
  }

  mReadAvailable -= length;
  return array;
}

function readUint16() {
  return readUint8() | readUint8() << 8;
}

function readUint32() {
  return readUint8()       | readUint8() <<  8 |
         readUint8() << 16 | readUint8() << 24;
}

function readUint32List() {
  let length = readUint32();
  let ints = [];
  for (let i = 0; i < length; i++) {
    ints.push(readUint32());
  }
  return ints;
}

function readString() {
  let string_len = readUint32();
  if (string_len < 0 || string_len >= INT32_MAX) {
    return null;
  }
  let s = "";
  for (let i = 0; i < string_len; i++) {
    s += String.fromCharCode(readUint16());
  }
  
  
  
  readStringDelimiter(string_len);
  return s;
}

function readStringList() {
  let num_strings = readUint32();
  let strings = [];
  for (let i = 0; i < num_strings; i++) {
    strings.push(readString());
  }
  return strings;
}

function readStringDelimiter(length) {
  let delimiter = readUint16();
  if (!(length & 1)) {
    delimiter |= readUint16();
  }
  if (DEBUG) {
    if (delimiter !== 0) {
      debug("Something's wrong, found string delimiter: " + delimiter);
    }
  }
}

function readParcelSize() {
  return readUint8Unchecked() << 24 |
         readUint8Unchecked() << 16 |
         readUint8Unchecked() <<  8 |
         readUint8Unchecked();
}












function ensureOutgoingAvailable(index) {
  if (index >= mOutgoingBufferLength) {
    growOutgoingBuffer(index + 1);
  }
}

function writeUint8(value) {
  ensureOutgoingAvailable(mOutgoingIndex);

  mOutgoingBytes[mOutgoingIndex] = value;
  mOutgoingIndex++;
}

function writeUint16(value) {
  writeUint8(value & 0xff);
  writeUint8((value >> 8) & 0xff);
}

function writeUint32(value) {
  writeUint8(value & 0xff);
  writeUint8((value >> 8) & 0xff);
  writeUint8((value >> 16) & 0xff);
  writeUint8((value >> 24) & 0xff);
}

function writeString(value) {
  if (value == null) {
    writeUint32(-1);
    return;
  }
  writeUint32(value.length);
  for (let i = 0; i < value.length; i++) {
    writeUint16(value.charCodeAt(i));
  }
  
  
  
  writeStringDelimiter(value.length);
}

function writeStringList(strings) {
  writeUint32(strings.length);
  for (let i = 0; i < strings.length; i++) {
    writeString(strings[i]);
  }
}

function writeStringDelimiter(length) {
  writeUint16(0);
  if (!(length & 1)) {
    writeUint16(0);
  }
}

function writeParcelSize(value) {
  




  let currentIndex = mOutgoingIndex;
  mOutgoingIndex = 0;
  writeUint8((value >> 24) & 0xff);
  writeUint8((value >> 16) & 0xff);
  writeUint8((value >> 8) & 0xff);
  writeUint8(value & 0xff);
  mOutgoingIndex = currentIndex;
}

function copyIncomingToOutgoing(length) {
  if (!length || (length < 0)) {
    return;
  }

  let translatedReadIndexEnd = mCurrentParcelSize - mReadAvailable + length - 1;
  ensureIncomingAvailable(translatedReadIndexEnd);

  let translatedWriteIndexEnd = mOutgoingIndex + length - 1;
  ensureOutgoingAvailable(translatedWriteIndexEnd);

  let newIncomingReadIndex = mIncomingReadIndex + length;
  if (newIncomingReadIndex < mIncomingBufferLength) {
    
    mOutgoingBytes.set(mIncomingBytes.subarray(mIncomingReadIndex, newIncomingReadIndex),
                           mOutgoingIndex);
  } else {
    
    newIncomingReadIndex %= mIncomingBufferLength;
    mOutgoingBytes.set(mIncomingBytes.subarray(mIncomingReadIndex, mIncomingBufferLength),
                           mOutgoingIndex);
    if (newIncomingReadIndex) {
      let firstPartLength = mIncomingBufferLength - mIncomingReadIndex;
      mOutgoingBytes.set(mIncomingBytes.subarray(0, newIncomingReadIndex),
                             mOutgoingIndex + firstPartLength);
    }
  }

  mIncomingReadIndex = newIncomingReadIndex;
  mReadAvailable -= length;
  mOutgoingIndex += length;
}











function writeToIncoming(incoming) {
  
  
  
  
  let minMustAvailableSize = incoming.length + mReadIncoming;
  if (minMustAvailableSize > mIncomingBufferLength) {
    growIncomingBuffer(minMustAvailableSize);
  }

  
  
  let remaining = mIncomingBufferLength - mIncomingWriteIndex;
  if (remaining >= incoming.length) {
    mIncomingBytes.set(incoming, mIncomingWriteIndex);
  } else {
    
    let head = incoming.subarray(0, remaining);
    let tail = incoming.subarray(remaining);
    mIncomingBytes.set(head, mIncomingWriteIndex);
    mIncomingBytes.set(tail, 0);
  }
  mIncomingWriteIndex = (mIncomingWriteIndex + incoming.length) %
                            mIncomingBufferLength;
}







function processIncoming(incoming) {
  if (DEBUG) {
    debug("Received " + incoming.length + " bytes.");
    debug("Already read " + mReadIncoming);
  }

  writeToIncoming(incoming);
  mReadIncoming += incoming.length;
  while (true) {
    if (!mCurrentParcelSize) {
      
      if (mReadIncoming < PARCEL_SIZE_SIZE) {
        
        
        if (DEBUG) debug("Next parcel size unknown, going to sleep.");
        return;
      }
      mCurrentParcelSize = readParcelSize();
      if (DEBUG) debug("New incoming parcel of size " +
                       mCurrentParcelSize);
      
      mReadIncoming -= PARCEL_SIZE_SIZE;
    }

    if (mReadIncoming < mCurrentParcelSize) {
      
      if (DEBUG) debug("Read " + mReadIncoming + ", but parcel size is "
                       + mCurrentParcelSize + ". Going to sleep.");
      return;
    }

    
    
    let expectedAfterIndex = (mIncomingReadIndex + mCurrentParcelSize)
                             % mIncomingBufferLength;

    if (DEBUG) {
      let parcel;
      if (expectedAfterIndex < mIncomingReadIndex) {
        let head = mIncomingBytes.subarray(mIncomingReadIndex);
        let tail = mIncomingBytes.subarray(0, expectedAfterIndex);
        parcel = Array.slice(head).concat(Array.slice(tail));
      } else {
        parcel = Array.slice(mIncomingBytes.subarray(
          mIncomingReadIndex, expectedAfterIndex));
      }
      debug("Parcel (size " + mCurrentParcelSize + "): " + parcel);
    }

    if (DEBUG) debug("We have at least one complete parcel.");
    try {
      mReadAvailable = mCurrentParcelSize;
      processParcel();
    } catch (ex) {
      if (DEBUG) debug("Parcel handling threw " + ex + "\n" + ex.stack);
    }

    
    if (mIncomingReadIndex != expectedAfterIndex) {
      if (DEBUG) {
        debug("Parcel handler didn't consume whole parcel, " +
              Math.abs(expectedAfterIndex - mIncomingReadIndex) +
              " bytes left over");
      }
      mIncomingReadIndex = expectedAfterIndex;
    }
    mReadIncoming -= mCurrentParcelSize;
    mReadAvailable = 0;
    mCurrentParcelSize = 0;
  }
}




function processParcel() {
  let response_type = readUint32();

  let request_type, options;
  if (response_type == RESPONSE_TYPE_SOLICITED) {
    let token = readUint32();
    let error = readUint32();

    options = mTokenRequestMap[token];
    if (!options) {
      if (DEBUG) {
        debug("Suspicious uninvited request found: " + token + ". Ignored!");
      }
      return;
    }

    delete mTokenRequestMap[token];
    request_type = options.rilRequestType;

    options.rilRequestError = error;
    if (DEBUG) {
      debug("Solicited response for request type " + request_type +
            ", token " + token + ", error " + error);
    }
  } else if (response_type == RESPONSE_TYPE_UNSOLICITED) {
    request_type = readUint32();
    if (DEBUG) debug("Unsolicited response for request type " + request_type);
  } else {
    if (DEBUG) debug("Unknown response type: " + response_type);
    return;
  }

  RIL.handleParcel(request_type, mReadAvailable, options);
}










function newParcel(type, options) {
  if (DEBUG) debug("New outgoing parcel of type " + type);

  
  mOutgoingIndex = PARCEL_SIZE_SIZE;
  writeUint32(type);
  writeUint32(mToken);

  if (!options) {
    options = {};
  }
  options.rilRequestType = type;
  options.rilRequestError = null;
  mTokenRequestMap[mToken] = options;
  mToken++;
  return mToken;
}




function sendParcel() {
  
  
  
  let parcelSize = mOutgoingIndex - PARCEL_SIZE_SIZE;
  writeParcelSize(parcelSize);

  
  
  let parcel = mOutgoingBytes.subarray(0, mOutgoingIndex);
  if (DEBUG) debug("Outgoing parcel: " + Array.slice(parcel));
  mOutputStream(parcel);
  mOutgoingIndex = PARCEL_SIZE_SIZE;
}

function setOutputStream(func) {
  mOutputStream = func;
}

function simpleRequest(type, options) {
  newParcel(type, options);
  sendParcel();
}

function getCurrentParcelSize() {
  return mCurrentParcelSize;
}

function getReadAvailable() {
  return mReadAvailable;
}

module.exports = {
  init: init,
  startCalOutgoingSize: startCalOutgoingSize,
  stopCalOutgoingSize: stopCalOutgoingSize,
  seekIncoming: seekIncoming,
  readUint8: readUint8,
  readUint8Array: readUint8Array,
  readUint16: readUint16,
  readUint32: readUint32,
  readUint32List: readUint32List,
  readString: readString,
  readStringList: readStringList,
  readStringDelimiter: readStringDelimiter,
  writeUint8: writeUint8,
  writeUint16: writeUint16,
  writeUint32: writeUint32,
  writeString: writeString,
  writeStringList: writeStringList,
  writeStringDelimiter: writeStringDelimiter,
  copyIncomingToOutgoing: copyIncomingToOutgoing,
  processIncoming: processIncoming,
  newParcel: newParcel,
  sendParcel: sendParcel,
  simpleRequest: simpleRequest,
  setOutputStream: setOutputStream,
  getCurrentParcelSize: getCurrentParcelSize,
  getReadAvailable: getReadAvailable,
};
