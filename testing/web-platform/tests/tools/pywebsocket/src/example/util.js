































var results = {};

function getTimeStamp() {
  return Date.now();
}

function formatResultInKiB(size, timePerMessageInMs, stddevTimePerMessageInMs,
    speed, printSize) {
  if (printSize) {
    return (size / 1024) +
        '\t' + timePerMessageInMs.toFixed(3) +
        (stddevTimePerMessageInMs == -1 ?
            '' :
            '\t' + stddevTimePerMessageInMs.toFixed(3)) +
        '\t' + speed.toFixed(3);
  } else {
    return speed.toString();
  }
}

function clearAverageData() {
  results = {};
}

function reportAverageData(config) {
  config.addToSummary(
      'Size[KiB]\tAverage time[ms]\tStddev time[ms]\tSpeed[KB/s]');
  for (var size in results) {
    var averageTimePerMessageInMs = results[size].sum_t / results[size].n;
    var speed = calculateSpeedInKB(size, averageTimePerMessageInMs);
    
    var stddevTimePerMessageInMs = Math.sqrt(
        (results[size].sum_t2 / results[size].n -
            averageTimePerMessageInMs * averageTimePerMessageInMs) *
        results[size].n /
        (results[size].n - 1));
    config.addToSummary(formatResultInKiB(
        size, averageTimePerMessageInMs, stddevTimePerMessageInMs, speed,
        true));
  }
}

function calculateSpeedInKB(size, timeSpentInMs) {
  return Math.round(size / timeSpentInMs * 1000) / 1000;
}

function calculateAndLogResult(config, size, startTimeInMs, totalSize) {
  var timeSpentInMs = getTimeStamp() - startTimeInMs;
  var speed = calculateSpeedInKB(totalSize, timeSpentInMs);
  var timePerMessageInMs = timeSpentInMs / (totalSize / size);
  if (!results[size]) {
    results[size] = {n: 0, sum_t: 0, sum_t2: 0};
  }
  config.measureValue(timePerMessageInMs);
  results[size].n ++;
  results[size].sum_t += timePerMessageInMs;
  results[size].sum_t2 += timePerMessageInMs * timePerMessageInMs;
  config.addToLog(formatResultInKiB(size, timePerMessageInMs, -1, speed,
      config.printSize));
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

function verifyBlob(config, blob, expectedChar, doneCallback) {
  var reader = new FileReader(blob);
  reader.onerror = function() {
    config.addToLog('FileReader Error: ' + reader.error.message);
    doneCallback(blob.size, false);
  }
  reader.onloadend = function() {
    var result = verifyArrayBuffer(reader.result, expectedChar);
    doneCallback(blob.size, result);
  }
  reader.readAsArrayBuffer(blob);
}

function verifyAcknowledgement(config, message, size) {
  if (typeof message != 'string') {
    config.addToLog('Invalid ack type: ' + typeof message);
    return false;
  }
  var parsedAck = parseInt(message);
  if (isNaN(parsedAck)) {
    config.addToLog('Invalid ack value: ' + message);
    return false;
  }
  if (parsedAck != size) {
    config.addToLog(
        'Expected ack for ' + size + 'B but received one for ' + parsedAck +
        'B');
    return false;
  }

  return true;
}

function cloneConfig(obj) {
  var newObj = {};
  for (key in obj) {
    newObj[key] = obj[key];
  }
  return newObj;
}
