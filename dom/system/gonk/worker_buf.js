

















let Buf = {
  INT32_MAX: 2147483647,
  UINT8_SIZE: 1,
  UINT16_SIZE: 2,
  UINT32_SIZE: 4,
  PARCEL_SIZE_SIZE: 4,
  PDU_HEX_OCTET_SIZE: 4,

  incomingBufferLength: 1024,
  incomingBuffer: null,
  incomingBytes: null,
  incomingWriteIndex: 0,
  incomingReadIndex: 0,
  readIncoming: 0,
  readAvailable: 0,
  currentParcelSize: 0,

  outgoingBufferLength: 1024,
  outgoingBuffer: null,
  outgoingBytes: null,
  outgoingIndex: 0,
  outgoingBufferCalSizeQueue: null,

  _init: function() {
    this.incomingBuffer = new ArrayBuffer(this.incomingBufferLength);
    this.outgoingBuffer = new ArrayBuffer(this.outgoingBufferLength);

    this.incomingBytes = new Uint8Array(this.incomingBuffer);
    this.outgoingBytes = new Uint8Array(this.outgoingBuffer);

    
    this.incomingWriteIndex = 0;
    this.incomingReadIndex = 0;

    
    this.outgoingIndex = this.PARCEL_SIZE_SIZE;

    
    this.readIncoming = 0;

    
    this.readAvailable = 0;

    
    
    this.currentParcelSize = 0;

    
    this.outgoingBufferCalSizeQueue = [];
  },

  











  startCalOutgoingSize: function(writeFunction) {
    let sizeInfo = {index: this.outgoingIndex,
                    write: writeFunction};

    
    writeFunction.call(0);

    
    sizeInfo.size = this.outgoingIndex - sizeInfo.index;

    
    this.outgoingBufferCalSizeQueue.push(sizeInfo);
  },

  


  stopCalOutgoingSize: function() {
    let sizeInfo = this.outgoingBufferCalSizeQueue.pop();

    
    let currentOutgoingIndex = this.outgoingIndex;
    
    let writeSize = this.outgoingIndex - sizeInfo.index - sizeInfo.size;

    
    
    this.outgoingIndex = sizeInfo.index;
    sizeInfo.write(writeSize);

    
    this.outgoingIndex = currentOutgoingIndex;
  },

  






  growIncomingBuffer: function(min_size) {
    if (DEBUG) {
      debug("Current buffer of " + this.incomingBufferLength +
            " can't handle incoming " + min_size + " bytes.");
    }
    let oldBytes = this.incomingBytes;
    this.incomingBufferLength =
      2 << Math.floor(Math.log(min_size)/Math.log(2));
    if (DEBUG) debug("New incoming buffer size: " + this.incomingBufferLength);
    this.incomingBuffer = new ArrayBuffer(this.incomingBufferLength);
    this.incomingBytes = new Uint8Array(this.incomingBuffer);
    if (this.incomingReadIndex <= this.incomingWriteIndex) {
      
      
      
      this.incomingBytes.set(oldBytes, 0);
    } else {
      
      
      
      
      let head = oldBytes.subarray(this.incomingReadIndex);
      let tail = oldBytes.subarray(0, this.incomingReadIndex);
      this.incomingBytes.set(head, 0);
      this.incomingBytes.set(tail, head.length);
      this.incomingReadIndex = 0;
      this.incomingWriteIndex += head.length;
    }
    if (DEBUG) {
      debug("New incoming buffer size is " + this.incomingBufferLength);
    }
  },

  






  growOutgoingBuffer: function(min_size) {
    if (DEBUG) {
      debug("Current buffer of " + this.outgoingBufferLength +
            " is too small.");
    }
    let oldBytes = this.outgoingBytes;
    this.outgoingBufferLength =
      2 << Math.floor(Math.log(min_size)/Math.log(2));
    this.outgoingBuffer = new ArrayBuffer(this.outgoingBufferLength);
    this.outgoingBytes = new Uint8Array(this.outgoingBuffer);
    this.outgoingBytes.set(oldBytes, 0);
    if (DEBUG) {
      debug("New outgoing buffer size is " + this.outgoingBufferLength);
    }
  },

  





  






  ensureIncomingAvailable: function(index) {
    if (index >= this.currentParcelSize) {
      throw new Error("Trying to read data beyond the parcel end!");
    } else if (index < 0) {
      throw new Error("Trying to read data before the parcel begin!");
    }
  },

  





  seekIncoming: function(offset) {
    
    let cur = this.currentParcelSize - this.readAvailable;

    let newIndex = cur + offset;
    this.ensureIncomingAvailable(newIndex);

    
    
    
    
    
    this.readAvailable = this.currentParcelSize - newIndex;

    
    if (this.incomingReadIndex < cur) {
      
      newIndex += this.incomingBufferLength;
    }
    newIndex += (this.incomingReadIndex - cur);
    newIndex %= this.incomingBufferLength;
    this.incomingReadIndex = newIndex;
  },

  readUint8Unchecked: function() {
    let value = this.incomingBytes[this.incomingReadIndex];
    this.incomingReadIndex = (this.incomingReadIndex + 1) %
                             this.incomingBufferLength;
    return value;
  },

  readUint8: function() {
    
    let cur = this.currentParcelSize - this.readAvailable;
    this.ensureIncomingAvailable(cur);

    this.readAvailable--;
    return this.readUint8Unchecked();
  },

  readUint8Array: function(length) {
    
    let last = this.currentParcelSize - this.readAvailable;
    last += (length - 1);
    this.ensureIncomingAvailable(last);

    let array = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = this.readUint8Unchecked();
    }

    this.readAvailable -= length;
    return array;
  },

  readUint16: function() {
    return this.readUint8() | this.readUint8() << 8;
  },

  readInt32: function() {
    return this.readUint8()       | this.readUint8() <<  8 |
           this.readUint8() << 16 | this.readUint8() << 24;
  },

  readInt64: function() {
    
    
    return this.readUint8()                   +
           this.readUint8() * Math.pow(2, 8)  +
           this.readUint8() * Math.pow(2, 16) +
           this.readUint8() * Math.pow(2, 24) +
           this.readUint8() * Math.pow(2, 32) +
           this.readUint8() * Math.pow(2, 40) +
           this.readUint8() * Math.pow(2, 48) +
           this.readUint8() * Math.pow(2, 56);
  },

  readInt32List: function() {
    let length = this.readInt32();
    let ints = [];
    for (let i = 0; i < length; i++) {
      ints.push(this.readInt32());
    }
    return ints;
  },

  readString: function() {
    let string_len = this.readInt32();
    if (string_len < 0 || string_len >= this.INT32_MAX) {
      return null;
    }
    let s = "";
    for (let i = 0; i < string_len; i++) {
      s += String.fromCharCode(this.readUint16());
    }
    
    
    
    this.readStringDelimiter(string_len);
    return s;
  },

  readStringList: function() {
    let num_strings = this.readInt32();
    let strings = [];
    for (let i = 0; i < num_strings; i++) {
      strings.push(this.readString());
    }
    return strings;
  },

  readStringDelimiter: function(length) {
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

  readParcelSize: function() {
    return this.readUint8Unchecked() << 24 |
           this.readUint8Unchecked() << 16 |
           this.readUint8Unchecked() <<  8 |
           this.readUint8Unchecked();
  },

  



  






  ensureOutgoingAvailable: function(index) {
    if (index >= this.outgoingBufferLength) {
      this.growOutgoingBuffer(index + 1);
    }
  },

  writeUint8: function(value) {
    this.ensureOutgoingAvailable(this.outgoingIndex);

    this.outgoingBytes[this.outgoingIndex] = value;
    this.outgoingIndex++;
  },

  writeUint16: function(value) {
    this.writeUint8(value & 0xff);
    this.writeUint8((value >> 8) & 0xff);
  },

  writeInt32: function(value) {
    this.writeUint8(value & 0xff);
    this.writeUint8((value >> 8) & 0xff);
    this.writeUint8((value >> 16) & 0xff);
    this.writeUint8((value >> 24) & 0xff);
  },

  writeString: function(value) {
    if (value == null) {
      this.writeInt32(-1);
      return;
    }
    this.writeInt32(value.length);
    for (let i = 0; i < value.length; i++) {
      this.writeUint16(value.charCodeAt(i));
    }
    
    
    
    this.writeStringDelimiter(value.length);
  },

  writeStringList: function(strings) {
    this.writeInt32(strings.length);
    for (let i = 0; i < strings.length; i++) {
      this.writeString(strings[i]);
    }
  },

  writeStringDelimiter: function(length) {
    this.writeUint16(0);
    if (!(length & 1)) {
      this.writeUint16(0);
    }
  },

  writeParcelSize: function(value) {
    




    let currentIndex = this.outgoingIndex;
    this.outgoingIndex = 0;
    this.writeUint8((value >> 24) & 0xff);
    this.writeUint8((value >> 16) & 0xff);
    this.writeUint8((value >> 8) & 0xff);
    this.writeUint8(value & 0xff);
    this.outgoingIndex = currentIndex;
  },

  copyIncomingToOutgoing: function(length) {
    if (!length || (length < 0)) {
      return;
    }

    let translatedReadIndexEnd =
      this.currentParcelSize - this.readAvailable + length - 1;
    this.ensureIncomingAvailable(translatedReadIndexEnd);

    let translatedWriteIndexEnd = this.outgoingIndex + length - 1;
    this.ensureOutgoingAvailable(translatedWriteIndexEnd);

    let newIncomingReadIndex = this.incomingReadIndex + length;
    if (newIncomingReadIndex < this.incomingBufferLength) {
      
      this.outgoingBytes
          .set(this.incomingBytes.subarray(this.incomingReadIndex,
                                           newIncomingReadIndex),
               this.outgoingIndex);
    } else {
      
      newIncomingReadIndex %= this.incomingBufferLength;
      this.outgoingBytes
          .set(this.incomingBytes.subarray(this.incomingReadIndex,
                                           this.incomingBufferLength),
               this.outgoingIndex);
      if (newIncomingReadIndex) {
        let firstPartLength = this.incomingBufferLength - this.incomingReadIndex;
        this.outgoingBytes.set(this.incomingBytes.subarray(0, newIncomingReadIndex),
                               this.outgoingIndex + firstPartLength);
      }
    }

    this.incomingReadIndex = newIncomingReadIndex;
    this.readAvailable -= length;
    this.outgoingIndex += length;
  },

  



  





  writeToIncoming: function(incoming) {
    
    
    
    
    let minMustAvailableSize = incoming.length + this.readIncoming;
    if (minMustAvailableSize > this.incomingBufferLength) {
      this.growIncomingBuffer(minMustAvailableSize);
    }

    
    
    let remaining = this.incomingBufferLength - this.incomingWriteIndex;
    if (remaining >= incoming.length) {
      this.incomingBytes.set(incoming, this.incomingWriteIndex);
    } else {
      
      let head = incoming.subarray(0, remaining);
      let tail = incoming.subarray(remaining);
      this.incomingBytes.set(head, this.incomingWriteIndex);
      this.incomingBytes.set(tail, 0);
    }
    this.incomingWriteIndex = (this.incomingWriteIndex + incoming.length) %
                               this.incomingBufferLength;
  },

  





  processIncoming: function(incoming) {
    if (DEBUG) {
      debug("Received " + incoming.length + " bytes.");
      debug("Already read " + this.readIncoming);
    }

    this.writeToIncoming(incoming);
    this.readIncoming += incoming.length;
    while (true) {
      if (!this.currentParcelSize) {
        
        if (this.readIncoming < this.PARCEL_SIZE_SIZE) {
          
          
          if (DEBUG) debug("Next parcel size unknown, going to sleep.");
          return;
        }
        this.currentParcelSize = this.readParcelSize();
        if (DEBUG) {
          debug("New incoming parcel of size " + this.currentParcelSize);
        }
        
        this.readIncoming -= this.PARCEL_SIZE_SIZE;
      }

      if (this.readIncoming < this.currentParcelSize) {
        
        if (DEBUG) debug("Read " + this.readIncoming + ", but parcel size is "
                         + this.currentParcelSize + ". Going to sleep.");
        return;
      }

      
      
      let expectedAfterIndex = (this.incomingReadIndex + this.currentParcelSize)
                               % this.incomingBufferLength;

      if (DEBUG) {
        let parcel;
        if (expectedAfterIndex < this.incomingReadIndex) {
          let head = this.incomingBytes.subarray(this.incomingReadIndex);
          let tail = this.incomingBytes.subarray(0, expectedAfterIndex);
          parcel = Array.slice(head).concat(Array.slice(tail));
        } else {
          parcel = Array.slice(this.incomingBytes.subarray(
            this.incomingReadIndex, expectedAfterIndex));
        }
        debug("Parcel (size " + this.currentParcelSize + "): " + parcel);
      }

      if (DEBUG) debug("We have at least one complete parcel.");
      try {
        this.readAvailable = this.currentParcelSize;
        this.processParcel();
      } catch (ex) {
        if (DEBUG) debug("Parcel handling threw " + ex + "\n" + ex.stack);
      }

      
      if (this.incomingReadIndex != expectedAfterIndex) {
        if (DEBUG) {
          debug("Parcel handler didn't consume whole parcel, " +
                Math.abs(expectedAfterIndex - this.incomingReadIndex) +
                " bytes left over");
        }
        this.incomingReadIndex = expectedAfterIndex;
      }
      this.readIncoming -= this.currentParcelSize;
      this.readAvailable = 0;
      this.currentParcelSize = 0;
    }
  },

  


  sendParcel: function() {
    
    
    
    let parcelSize = this.outgoingIndex - this.PARCEL_SIZE_SIZE;
    this.writeParcelSize(parcelSize);

    
    
    let parcel = this.outgoingBytes.subarray(0, this.outgoingIndex);
    if (DEBUG) debug("Outgoing parcel: " + Array.slice(parcel));
    this.onSendParcel(parcel);
    this.outgoingIndex = this.PARCEL_SIZE_SIZE;
  },

  getCurrentParcelSize: function() {
    return this.currentParcelSize;
  },

  getReadAvailable: function() {
    return this.readAvailable;
  }

  






  
  
  
  

  










  
  
  
};

module.exports = { Buf: Buf };
