


"use strict";

this.EXPORTED_SYMBOLS = ["LogParser"];








function parseLogArray(array) {
  let data = new DataView(array.buffer);
  let byteString = String.fromCharCode.apply(null, array);

  let logMessages = [];
  let pos = 0;

  while (pos < byteString.length) {
    

    
    let offset = 0;

    
    let length = data.getUint32(pos + offset, true);
    offset += 4;
    
    let processId = data.getUint32(pos + offset, true);
    offset += 4;
    
    let threadId = data.getUint32(pos + offset, true);
    offset += 4;
    
    let seconds = data.getUint32(pos + offset, true);
    offset += 4;
    
    let nanoseconds = data.getUint32(pos + offset, true);
    offset += 4;

    
    
    let priority = data.getUint8(pos + offset);

    
    pos += offset;
    offset = 0;
    offset += 1;

    
    let tag = "";
    while (byteString[pos + offset] != "\0") {
      tag += byteString[pos + offset];
      offset ++;
    }
    offset ++;

    let message = "";
    
    while (byteString[pos + offset] != "\0" && offset < length) {
      message += byteString[pos + offset];
      offset ++;
    }

    
    if (offset === length) {
      offset --;
    }

    offset ++;

    pos += offset;

    
    
    if (message.charAt(message.length - 1) === "\n") {
      message = message.substring(0, message.length - 1);
    }

    
    
    let time = seconds * 1000.0 + nanoseconds/1000000.0;

    
    
    for (let lineMessage of message.split("\n")) {
      logMessages.push({
        processId: processId,
        threadId: threadId,
        seconds: seconds,
        nanoseconds: nanoseconds,
        time: time,
        priority: priority,
        tag: tag,
        message: lineMessage + "\n"
      });
    }
  }

  return logMessages;
}






function getTimeString(time) {
  let date = new Date(time);
  function pad(number) {
    if ( number < 10 ) {
      return "0" + number;
    }
    return number;
  }
  return pad( date.getMonth() + 1 ) +
         "-" + pad( date.getDate() ) +
         " " + pad( date.getHours() ) +
         ":" + pad( date.getMinutes() ) +
         ":" + pad( date.getSeconds() ) +
         "." + (date.getMilliseconds() / 1000).toFixed(3).slice(2, 5);
}






function padLeft(str, width) {
  while (str.length < width) {
    str = " " + str;
  }
  return str;
}






function padRight(str, width) {
  while (str.length < width) {
    str = str + " ";
  }
  return str;
}


const ANDROID_LOG_UNKNOWN = 0;
const ANDROID_LOG_DEFAULT = 1;
const ANDROID_LOG_VERBOSE = 2;
const ANDROID_LOG_DEBUG   = 3;
const ANDROID_LOG_INFO    = 4;
const ANDROID_LOG_WARN    = 5;
const ANDROID_LOG_ERROR   = 6;
const ANDROID_LOG_FATAL   = 7;
const ANDROID_LOG_SILENT  = 8;






function getPriorityString(priorityNumber) {
  switch (priorityNumber) {
  case ANDROID_LOG_VERBOSE:
    return "V";
  case ANDROID_LOG_DEBUG:
    return "D";
  case ANDROID_LOG_INFO:
    return "I";
  case ANDROID_LOG_WARN:
    return "W";
  case ANDROID_LOG_ERROR:
    return "E";
  case ANDROID_LOG_FATAL:
    return "F";
  case ANDROID_LOG_SILENT:
    return "S";
  default:
    return "?";
  }
}








function formatLogMessage(logMessage) {
  
  
  return getTimeString(logMessage.time) +
         " " + padLeft(""+logMessage.processId, 5) +
         " " + padLeft(""+logMessage.threadId, 5) +
         " " + getPriorityString(logMessage.priority) +
         " " + padRight(logMessage.tag, 8) +
         ": " + logMessage.message;
}







function prettyPrintLogArray(array) {
  let logMessages = parseLogArray(array);
  return logMessages.map(formatLogMessage).join("");
}







function parsePropertiesArray(array) {
  let data = new DataView(array.buffer);
  let byteString = String.fromCharCode.apply(null, array);

  let properties = {};

  let propIndex = 0;
  let propCount = data.getUint32(0, true);

  
  let tocOffset = 32;

  const PROP_NAME_MAX = 32;
  const PROP_VALUE_MAX = 92;

  while (propIndex < propCount) {
    
    let infoOffset = data.getUint32(tocOffset, true) & 0xffffff;

    
    let propName = "";
    let nameOffset = infoOffset;
    while (byteString[nameOffset] != "\0" &&
           (nameOffset - infoOffset) < PROP_NAME_MAX) {
      propName += byteString[nameOffset];
      nameOffset ++;
    }

    infoOffset += PROP_NAME_MAX;
    
    infoOffset += 4;

    let propValue = "";
    nameOffset = infoOffset;
    while (byteString[nameOffset] != "\0" &&
           (nameOffset - infoOffset) < PROP_VALUE_MAX) {
      propValue += byteString[nameOffset];
      nameOffset ++;
    }

    
    tocOffset += 4;

    properties[propName] = propValue;
    propIndex += 1;
  }

  return properties;
}






function prettyPrintPropertiesArray(array) {
  let properties = parsePropertiesArray(array);
  let propertiesString = "";
  for(let propName in properties) {
    propertiesString += propName + ": " + properties[propName] + "\n";
  }
  return propertiesString;
}





function prettyPrintArray(array) {
  return array;
}

this.LogParser = {
  parseLogArray: parseLogArray,
  parsePropertiesArray: parsePropertiesArray,
  prettyPrintArray: prettyPrintArray,
  prettyPrintLogArray: prettyPrintLogArray,
  prettyPrintPropertiesArray: prettyPrintPropertiesArray
};
