
































var logBox = null;
var queuedLog = '';

function queueLog(log) {
  queuedLog += log + '\n';
}

function addToLog(log) {
  logBox.value += queuedLog;
  queuedLog = '';
  logBox.value += log + '\n';
  logBox.scrollTop = 1000000;
}

function getTimeStamp() {
  return Date.now();
}

function formatResultInKiB(size, speed, printSize) {
  if (printSize) {
    return (size / 1024) + '\t' + speed;
  } else {
    return speed.toString();
  }
}

function calculateSpeedInKB(size, startTimeInMs) {
  return Math.round(size / (getTimeStamp() - startTimeInMs) * 1000) / 1000;
}

function calculateAndLogResult(size, startTimeInMs, totalSize, printSize) {
  var speed = calculateSpeedInKB(totalSize, startTimeInMs);
  addToLog(formatResultInKiB(size, speed, printSize));
}

function fillArrayBuffer(buffer, c) {
  var i;

  var u32Content = c * 0x01010101;

  var u32Blocks = Math.floor(buffer.byteLength / 4);
  var u32View = new Uint32Array(buffer, 0, u32Blocks);
  
  for (i = 0; i < u32Blocks; ++i) {
    u32View[i] = u32Content;
  }

  
  var u8Blocks = buffer.byteLength - u32Blocks * 4;
  var u8View = new Uint8Array(buffer, u32Blocks * 4, u8Blocks);
  for (i = 0; i < u8Blocks; ++i) {
    u8View[i] = c;
  }
}

function verifyArrayBuffer(buffer, expectedChar) {
  var i;

  var expectedU32Value = expectedChar * 0x01010101;

  var u32Blocks = Math.floor(buffer.byteLength / 4);
  var u32View = new Uint32Array(buffer, 0, u32Blocks);
  for (i = 0; i < u32Blocks; ++i) {
    if (u32View[i] != expectedU32Value) {
      return false;
    }
  }

  var u8Blocks = buffer.byteLength - u32Blocks * 4;
  var u8View = new Uint8Array(buffer, u32Blocks * 4, u8Blocks);
  for (i = 0; i < u8Blocks; ++i) {
    if (u8View[i] != expectedChar) {
      return false;
    }
  }

  return true;
}

function verifyBlob(blob, expectedChar, doneCallback) {
  var reader = new FileReader(blob);
  reader.onerror = function() {
    doneCallback(blob.size, false);
  }
  reader.onloadend = function() {
    var result = verifyArrayBuffer(reader.result, expectedChar);
    doneCallback(blob.size, result);
  }
  reader.readAsArrayBuffer(blob);
}

function verifyAcknowledgement(message, size) {
  if (typeof message != 'string') {
    addToLog('Invalid ack type: ' + typeof message);
    return false;
  }
  var parsedAck = parseInt(message);
  if (isNaN(parsedAck)) {
    addToLog('Invalid ack value: ' + message);
    return false;
  }
  if (parsedAck != size) {
    addToLog(
        'Expected ack for ' + size + 'B but received one for ' + parsedAck +
        'B');
    return false;
  }

  return true;
}
