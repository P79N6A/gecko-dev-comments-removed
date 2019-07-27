


"use strict";

const { Ci } = require("chrome");
const { extend } = require("sdk/util/object");
loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "CATEGORY_OTHER",
  "devtools/shared/profiler/global", true);


const CHAR_CODE_A = "a".charCodeAt(0);
const CHAR_CODE_C = "c".charCodeAt(0);
const CHAR_CODE_E = "e".charCodeAt(0);
const CHAR_CODE_F = "f".charCodeAt(0);
const CHAR_CODE_H = "h".charCodeAt(0);
const CHAR_CODE_I = "i".charCodeAt(0);
const CHAR_CODE_J = "j".charCodeAt(0);
const CHAR_CODE_L = "l".charCodeAt(0);
const CHAR_CODE_M = "m".charCodeAt(0);
const CHAR_CODE_O = "o".charCodeAt(0);
const CHAR_CODE_P = "p".charCodeAt(0);
const CHAR_CODE_R = "r".charCodeAt(0);
const CHAR_CODE_S = "s".charCodeAt(0);
const CHAR_CODE_T = "t".charCodeAt(0);
const CHAR_CODE_U = "u".charCodeAt(0);
const CHAR_CODE_0 = "0".charCodeAt(0);
const CHAR_CODE_9 = "9".charCodeAt(0);

const CHAR_CODE_LPAREN = "(".charCodeAt(0);
const CHAR_CODE_COLON = ":".charCodeAt(0);
const CHAR_CODE_SLASH = "/".charCodeAt(0);


const gNSURLStore = new Map();





exports.parseLocation = function parseLocation(location, fallbackLine, fallbackColumn) {
  

  let line, column, url;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  let firstParenIndex = -1;
  let lineAndColumnIndex = -1;

  
  
  for (let i = 0; i < location.length; i++) {
    let c = location.charCodeAt(i);

    
    if (c === CHAR_CODE_LPAREN) {
      if (firstParenIndex < 0) {
        firstParenIndex = i;
      }
      continue;
    }

    
    
    if (c === CHAR_CODE_COLON) {
      if (isNumeric(location.charCodeAt(i + 1))) {
        
        if (lineAndColumnIndex < 0) {
          lineAndColumnIndex = i;
        }

        let start = ++i;
        let length = 1;
        while (isNumeric(location.charCodeAt(++i))) {
          length++;
        }

        if (!line) {
          line = location.substr(start, length);
        } else if (!column) {
          column = location.substr(start, length);
        }

        break;
      }
    }
  }

  let uri;
  if (lineAndColumnIndex > 0) {
    let resource = location.substring(firstParenIndex + 1, lineAndColumnIndex);
    url = resource.split(" -> ").pop();
    if (url) {
      uri = nsIURL(url);
    }
  }

  let functionName, fileName, hostName;

  
  if (uri) {
    functionName = location.substring(0, firstParenIndex - 1);
    fileName = (uri.fileName + (uri.ref ? "#" + uri.ref : "")) || "/";
    hostName = getHost(url, uri.host);
  } else {
    functionName = location;
    url = null;
  }

  return {
    functionName: functionName,
    fileName: fileName,
    hostName: hostName,
    url: url,
    line: line,
    column: column
  };
};











function isContent({ location, category }) {
  
  if (category) {
    return false;
  }

  
  
  
  
  for (let i = 0; i < location.length; i++) {
    if (location.charCodeAt(i) === CHAR_CODE_LPAREN) {
      return isContentScheme(location, i + 1);
    }
  }

  
  return isContentScheme(location, 0);
}
exports.isContent = isContent;
























exports.filterPlatformData = function filterPlatformData (frames) {
  let result = [];
  let last = frames.length - 1;
  let frame;

  for (let i = 0; i < frames.length; i++) {
    frame = frames[i];
    if (exports.isContent(frame)) {
      result.push(frame);
    } else if (last === i) {
      
      
      
      
      result.push(extend({ isMetaCategory: true, category: CATEGORY_OTHER }, frame));
    }
  }

  return result;
}




function nsIURL(url) {
  let cached = gNSURLStore.get(url);
  
  
  if (cached !== void 0) {
    return cached;
  }
  let uri = null;
  try {
    uri = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
    
    
    uri.host;
  } catch(e) {
    
    uri = null;
  }
  gNSURLStore.set(url, uri);
  return uri;
};





function getHost (url, hostName) {
  return isChromeScheme(url) ? null : hostName;
}







function isColonSlashSlash(location, i) {
  return location.charCodeAt(++i) === CHAR_CODE_COLON &&
         location.charCodeAt(++i) === CHAR_CODE_SLASH &&
         location.charCodeAt(++i) === CHAR_CODE_SLASH;
}

function isContentScheme(location, i) {
  let firstChar = location.charCodeAt(i);

  switch (firstChar) {
  case CHAR_CODE_H: 
    if (location.charCodeAt(++i) === CHAR_CODE_T &&
        location.charCodeAt(++i) === CHAR_CODE_T &&
        location.charCodeAt(++i) === CHAR_CODE_P) {
      if (location.charCodeAt(i) === CHAR_CODE_S) {
        ++i;
      }
      return isColonSlashSlash(location, i);
    }
    return false;

  case CHAR_CODE_F: 
    if (location.charCodeAt(++i) === CHAR_CODE_I &&
        location.charCodeAt(++i) === CHAR_CODE_L &&
        location.charCodeAt(++i) === CHAR_CODE_E) {
      return isColonSlashSlash(location, i);
    }
    return false;

  case CHAR_CODE_A: 
    if (location.charCodeAt(++i) == CHAR_CODE_P &&
        location.charCodeAt(++i) == CHAR_CODE_P) {
      return isColonSlashSlash(location, i);
    }
    return false;

  default:
    return false;
  }
}

function isChromeScheme(location, i) {
  let firstChar = location.charCodeAt(i);

  switch (firstChar) {
  case CHAR_CODE_C: 
    if (location.charCodeAt(++i) === CHAR_CODE_H &&
        location.charCodeAt(++i) === CHAR_CODE_R &&
        location.charCodeAt(++i) === CHAR_CODE_O &&
        location.charCodeAt(++i) === CHAR_CODE_M &&
        location.charCodeAt(++i) === CHAR_CODE_E) {
      return isColonSlashSlash(location, i);
    }
    return false;

  case CHAR_CODE_R: 
    if (location.charCodeAt(++i) === CHAR_CODE_E &&
        location.charCodeAt(++i) === CHAR_CODE_S &&
        location.charCodeAt(++i) === CHAR_CODE_O &&
        location.charCodeAt(++i) === CHAR_CODE_U &&
        location.charCodeAt(++i) === CHAR_CODE_R &&
        location.charCodeAt(++i) === CHAR_CODE_C &&
        location.charCodeAt(++i) === CHAR_CODE_E) {
      return isColonSlashSlash(location, i);
    }
    return false;

  case CHAR_CODE_J: 
    if (location.charCodeAt(++i) === CHAR_CODE_A &&
        location.charCodeAt(++i) === CHAR_CODE_R &&
        location.charCodeAt(++i) === CHAR_CODE_COLON &&
        location.charCodeAt(++i) === CHAR_CODE_F &&
        location.charCodeAt(++i) === CHAR_CODE_I &&
        location.charCodeAt(++i) === CHAR_CODE_L &&
        location.charCodeAt(++i) === CHAR_CODE_E) {
      return isColonSlashSlash(location, i);
    }
    return false;

  default:
    return false;
  }
}

function isNumeric(c) {
  return c >= CHAR_CODE_0 && c <= CHAR_CODE_9;
}
