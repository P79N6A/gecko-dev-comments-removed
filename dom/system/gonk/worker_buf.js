



const INT32_MAX   = 2147483647;
const UINT8_SIZE  = 1;
const UINT16_SIZE = 2;
const UINT32_SIZE = 4;














let Buf = {
  PARCEL_SIZE_SIZE: UINT32_SIZE,

  mIncomingBufferLength: 1024,
  mIncomingBuffer: null,
  mIncomingBytes: null,
  mIncomingWriteIndex: 0,
  mIncomingReadIndex: 0,
  mReadIncoming: 0,
  mReadAvailable: 0,
  mCurrentParcelSize: 0,

  mOutgoingBufferLength: 1024,
  mOutgoingBuffer: null,
  mOutgoingBytes: null,
  mOutgoingIndex: 0,
  mOutgoingBufferCalSizeQueue: null,

  _init: function _init() {
    this.mIncomingBuffer = new ArrayBuffer(this.mIncomingBufferLength);
    this.mOutgoingBuffer = new ArrayBuffer(this.mOutgoingBufferLength);

    this.mIncomingBytes = new Uint8Array(this.mIncomingBuffer);
    this.mOutgoingBytes = new Uint8Array(this.mOutgoingBuffer);

    
    this.mIncomingWriteIndex = 0;
    this.mIncomingReadIndex = 0;

    
    this.mOutgoingIndex = this.PARCEL_SIZE_SIZE;

    
    this.mReadIncoming = 0;

    
    this.mReadAvailable = 0;

    
    
    this.mCurrentParcelSize = 0;

    
    this.mOutgoingBufferCalSizeQueue = [];
  },

  











  startCalOutgoingSize: function startCalOutgoingSize(writeFunction) {
    let sizeInfo = {index: this.mOutgoingIndex,
                    write: writeFunction};

    
    writeFunction.call(0);

    
    sizeInfo.size = this.mOutgoingIndex - sizeInfo.index;

    
    this.mOutgoingBufferCalSizeQueue.push(sizeInfo);
  },

  


  stopCalOutgoingSize: function stopCalOutgoingSize() {
    let sizeInfo = this.mOutgoingBufferCalSizeQueue.pop();

    
    let currentOutgoingIndex = this.mOutgoingIndex;
    
    let writeSize = this.mOutgoingIndex - sizeInfo.index - sizeInfo.size;

    
    
    this.mOutgoingIndex = sizeInfo.index;
    sizeInfo.write(writeSize);

    
    this.mOutgoingIndex = currentOutgoingIndex;
  },

  






  growIncomingBuffer: function growIncomingBuffer(min_size) {
    if (DEBUG) {
      debug("Current buffer of " + this.mIncomingBufferLength +
            " can't handle incoming " + min_size + " bytes.");
    }
    let oldBytes = this.mIncomingBytes;
    this.mIncomingBufferLength =
      2 << Math.floor(Math.log(min_size)/Math.log(2));
    if (DEBUG) debug("New incoming buffer size: " + this.mIncomingBufferLength);
    this.mIncomingBuffer = new ArrayBuffer(this.mIncomingBufferLength);
    this.mIncomingBytes = new Uint8Array(this.mIncomingBuffer);
    if (this.mIncomingReadIndex <= this.mIncomingWriteIndex) {
      
      
      
      this.mIncomingBytes.set(oldBytes, 0);
    } else {
      
      
      
      
      let head = oldBytes.subarray(this.mIncomingReadIndex);
      let tail = oldBytes.subarray(0, this.mIncomingReadIndex);
      this.mIncomingBytes.set(head, 0);
      this.mIncomingBytes.set(tail, head.length);
      this.mIncomingReadIndex = 0;
      this.mIncomingWriteIndex += head.length;
    }
    if (DEBUG) {
      debug("New incoming buffer size is " + this.mIncomingBufferLength);
    }
  },

  






  growOutgoingBuffer: function growOutgoingBuffer(min_size) {
    if (DEBUG) {
      debug("Current buffer of " + this.mOutgoingBufferLength +
            " is too small.");
    }
    let oldBytes = this.mOutgoingBytes;
    this.mOutgoingBufferLength =
      2 << Math.floor(Math.log(min_size)/Math.log(2));
    this.mOutgoingBuffer = new ArrayBuffer(this.mOutgoingBufferLength);
    this.mOutgoingBytes = new Uint8Array(this.mOutgoingBuffer);
    this.mOutgoingBytes.set(oldBytes, 0);
    if (DEBUG) {
      debug("New outgoing buffer size is " + this.mOutgoingBufferLength);
    }
  },

  





  






  ensureIncomingAvailable: function ensureIncomingAvailable(index) {
    if (index >= this.mCurrentParcelSize) {
      throw new Error("Trying to read data beyond the parcel end!");
    } else if (index < 0) {
      throw new Error("Trying to read data before the parcel begin!");
    }
  },

  





  seekIncoming: function seekIncoming(offset) {
    
    let cur = this.mCurrentParcelSize - this.mReadAvailable;

    let newIndex = cur + offset;
    this.ensureIncomingAvailable(newIndex);

    
    
    
    
    
    this.mReadAvailable = this.mCurrentParcelSize - newIndex;

    
    if (this.mIncomingReadIndex < cur) {
      
      newIndex += this.mIncomingBufferLength;
    }
    newIndex += (this.mIncomingReadIndex - cur);
    newIndex %= this.mIncomingBufferLength;
    this.mIncomingReadIndex = newIndex;
  },

  readUint8Unchecked: function readUint8Unchecked() {
    let value = this.mIncomingBytes[this.mIncomingReadIndex];
    this.mIncomingReadIndex = (this.mIncomingReadIndex + 1) %
                             this.mIncomingBufferLength;
    return value;
  },

  readUint8: function readUint8() {
    
    let cur = this.mCurrentParcelSize - this.mReadAvailable;
    this.ensureIncomingAvailable(cur);

    this.mReadAvailable--;
    return this.readUint8Unchecked();
  },

  readUint8Array: function readUint8Array(length) {
    
    let last = this.mCurrentParcelSize - this.mReadAvailable;
    last += (length - 1);
    this.ensureIncomingAvailable(last);

    let array = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = this.readUint8Unchecked();
    }

    this.mReadAvailable -= length;
    return array;
  },

  readUint16: function readUint16() {
    return this.readUint8() | this.readUint8() << 8;
  },

  readUint32: function readUint32() {
    return this.readUint8()       | this.readUint8() <<  8 |
           this.readUint8() << 16 | this.readUint8() << 24;
  },

  readUint32List: function readUint32List() {
    let length = this.readUint32();
    let ints = [];
    for (let i = 0; i < length; i++) {
      ints.push(this.readUint32());
    }
    return ints;
  },

  readString: function readString() {
    let string_len = this.readUint32();
    if (string_len < 0 || string_len >= INT32_MAX) {
      return null;
    }
    let s = "";
    for (let i = 0; i < string_len; i++) {
      s += String.fromCharCode(this.readUint16());
    }
    
    
    
    this.readStringDelimiter(string_len);
    return s;
  },

  readStringList: function readStringList() {
    let num_strings = this.readUint32();
    let strings = [];
    for (let i = 0; i < num_strings; i++) {
      strings.push(this.readString());
    }
    return strings;
  },

  readStringDelimiter: function readStringDelimiter(length) {
    let delimiter = this.readUint16();
    if (!(length & 1)) {
      delimiter |= this.readUint16();
    }
    if (DEBUG) {
      if (delimiter !== 0) {
        debug("Something's wrong, found string delimiter: " + delimiter);
      }
    }
  },

  readParcelSize: function readParcelSize() {
    return this.readUint8Unchecked() << 24 |
           this.readUint8Unchecked() << 16 |
           this.readUint8Unchecked() <<  8 |
           this.readUint8Unchecked();
  },

  



  






  ensureOutgoingAvailable: function ensureOutgoingAvailable(index) {
    if (index >= this.mOutgoingBufferLength) {
      this.growOutgoingBuffer(index + 1);
    }
  },

  writeUint8: function writeUint8(value) {
    this.ensureOutgoingAvailable(this.mOutgoingIndex);

    this.mOutgoingBytes[this.mOutgoingIndex] = value;
    this.mOutgoingIndex++;
  },

  writeUint16: function writeUint16(value) {
    this.writeUint8(value & 0xff);
    this.writeUint8((value >> 8) & 0xff);
  },

  writeUint32: function writeUint32(value) {
    this.writeUint8(value & 0xff);
    this.writeUint8((value >> 8) & 0xff);
    this.writeUint8((value >> 16) & 0xff);
    this.writeUint8((value >> 24) & 0xff);
  },

  writeString: function writeString(value) {
    if (value == null) {
      this.writeUint32(-1);
      return;
    }
    this.writeUint32(value.length);
    for (let i = 0; i < value.length; i++) {
      this.writeUint16(value.charCodeAt(i));
    }
    
    
    
    this.writeStringDelimiter(value.length);
  },

  writeStringList: function writeStringList(strings) {
    this.writeUint32(strings.length);
    for (let i = 0; i < strings.length; i++) {
      this.writeString(strings[i]);
    }
  },

  writeStringDelimiter: function writeStringDelimiter(length) {
    this.writeUint16(0);
    if (!(length & 1)) {
      this.writeUint16(0);
    }
  },

  writeParcelSize: function writeParcelSize(value) {
    




    let currentIndex = this.mOutgoingIndex;
    this.mOutgoingIndex = 0;
    this.writeUint8((value >> 24) & 0xff);
    this.writeUint8((value >> 16) & 0xff);
    this.writeUint8((value >> 8) & 0xff);
    this.writeUint8(value & 0xff);
    this.mOutgoingIndex = currentIndex;
  },

  copyIncomingToOutgoing: function copyIncomingToOutgoing(length) {
    if (!length || (length < 0)) {
      return;
    }

    let translatedReadIndexEnd =
      this.mCurrentParcelSize - this.mReadAvailable + length - 1;
    this.ensureIncomingAvailable(translatedReadIndexEnd);

    let translatedWriteIndexEnd = this.mOutgoingIndex + length - 1;
    this.ensureOutgoingAvailable(translatedWriteIndexEnd);

    let newIncomingReadIndex = this.mIncomingReadIndex + length;
    if (newIncomingReadIndex < this.mIncomingBufferLength) {
      
      this.mOutgoingBytes
          .set(this.mIncomingBytes.subarray(this.mIncomingReadIndex,
                                            newIncomingReadIndex),
               this.mOutgoingIndex);
    } else {
      
      newIncomingReadIndex %= this.mIncomingBufferLength;
      this.mOutgoingBytes
          .set(this.mIncomingBytes.subarray(this.mIncomingReadIndex,
                                            this.mIncomingBufferLength),
               this.mOutgoingIndex);
      if (newIncomingReadIndex) {
        let firstPartLength = this.mIncomingBufferLength - this.mIncomingReadIndex;
        this.mOutgoingBytes.set(this.mIncomingBytes.subarray(0, newIncomingReadIndex),
                               this.mOutgoingIndex + firstPartLength);
      }
    }

    this.mIncomingReadIndex = newIncomingReadIndex;
    this.mReadAvailable -= length;
    this.mOutgoingIndex += length;
  },

  



  





  writeToIncoming: function writeToIncoming(incoming) {
    
    
    
    
    let minMustAvailableSize = incoming.length + this.mReadIncoming;
    if (minMustAvailableSize > this.mIncomingBufferLength) {
      this.growIncomingBuffer(minMustAvailableSize);
    }

    
    
    let remaining = this.mIncomingBufferLength - this.mIncomingWriteIndex;
    if (remaining >= incoming.length) {
      this.mIncomingBytes.set(incoming, this.mIncomingWriteIndex);
    } else {
      
      let head = incoming.subarray(0, remaining);
      let tail = incoming.subarray(remaining);
      this.mIncomingBytes.set(head, this.mIncomingWriteIndex);
      this.mIncomingBytes.set(tail, 0);
    }
    this.mIncomingWriteIndex = (this.mIncomingWriteIndex + incoming.length) %
                               this.mIncomingBufferLength;
  },

  





  processIncoming: function processIncoming(incoming) {
    if (DEBUG) {
      debug("Received " + incoming.length + " bytes.");
      debug("Already read " + this.mReadIncoming);
    }

    this.writeToIncoming(incoming);
    this.mReadIncoming += incoming.length;
    while (true) {
      if (!this.mCurrentParcelSize) {
        
        if (this.mReadIncoming < this.PARCEL_SIZE_SIZE) {
          
          
          if (DEBUG) debug("Next parcel size unknown, going to sleep.");
          return;
        }
        this.mCurrentParcelSize = this.readParcelSize();
        if (DEBUG) {
          debug("New incoming parcel of size " + this.mCurrentParcelSize);
        }
        
        this.mReadIncoming -= this.PARCEL_SIZE_SIZE;
      }

      if (this.mReadIncoming < this.mCurrentParcelSize) {
        
        if (DEBUG) debug("Read " + this.mReadIncoming + ", but parcel size is "
                         + this.mCurrentParcelSize + ". Going to sleep.");
        return;
      }

      
      
      let expectedAfterIndex = (this.mIncomingReadIndex + this.mCurrentParcelSize)
                               % this.mIncomingBufferLength;

      if (DEBUG) {
        let parcel;
        if (expectedAfterIndex < this.mIncomingReadIndex) {
          let head = this.mIncomingBytes.subarray(this.mIncomingReadIndex);
          let tail = this.mIncomingBytes.subarray(0, expectedAfterIndex);
          parcel = Array.slice(head).concat(Array.slice(tail));
        } else {
          parcel = Array.slice(this.mIncomingBytes.subarray(
            this.mIncomingReadIndex, expectedAfterIndex));
        }
        debug("Parcel (size " + this.mCurrentParcelSize + "): " + parcel);
      }

      if (DEBUG) debug("We have at least one complete parcel.");
      try {
        this.mReadAvailable = this.mCurrentParcelSize;
        this.processParcel();
      } catch (ex) {
        if (DEBUG) debug("Parcel handling threw " + ex + "\n" + ex.stack);
      }

      
      if (this.mIncomingReadIndex != expectedAfterIndex) {
        if (DEBUG) {
          debug("Parcel handler didn't consume whole parcel, " +
                Math.abs(expectedAfterIndex - this.mIncomingReadIndex) +
                " bytes left over");
        }
        this.mIncomingReadIndex = expectedAfterIndex;
      }
      this.mReadIncoming -= this.mCurrentParcelSize;
      this.mReadAvailable = 0;
      this.mCurrentParcelSize = 0;
    }
  },

  


  sendParcel: function sendParcel() {
    
    
    
    let parcelSize = this.mOutgoingIndex - this.PARCEL_SIZE_SIZE;
    this.writeParcelSize(parcelSize);

    
    
    let parcel = this.mOutgoingBytes.subarray(0, this.mOutgoingIndex);
    if (DEBUG) debug("Outgoing parcel: " + Array.slice(parcel));
    this.onSendParcel(parcel);
    this.mOutgoingIndex = this.PARCEL_SIZE_SIZE;
  },

  getCurrentParcelSize: function getCurrentParcelSize() {
    return this.mCurrentParcelSize;
  },

  getReadAvailable: function getReadAvailable() {
    return this.mReadAvailable;
  }

  






  
  
  
  

  










  
  
  
};

module.exports = { Buf: Buf };
