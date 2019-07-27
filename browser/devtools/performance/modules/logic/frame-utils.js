


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "global",
  "devtools/performance/global");


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


const gInflatedFrameStore = new WeakMap();





function parseLocation(location, fallbackLine, fallbackColumn) {
  

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

        
        if (location.charCodeAt(i) === CHAR_CODE_SLASH) {
          lineAndColumnIndex = -1;
          --i;
          continue;
        }

        if (!line) {
          line = location.substr(start, length);

          
          --i;

          
          continue;
        }

        column = location.substr(start, length);

        
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

  let functionName, fileName, hostName, port, host;
  line = line || fallbackLine;
  column = column || fallbackColumn;

  
  if (uri) {
    functionName = location.substring(0, firstParenIndex - 1);
    fileName = uri.fileName || "/";
    hostName = getHost(url, uri.host);
    
    
    
    if (hostName) {
      try {
        port = uri.port === -1 ? null : uri.port;
        host = port !== null ? `${hostName}:${port}` : hostName;
      } catch (e) {
        host = hostName;
      }
    }
  } else {
    functionName = location;
    url = null;
  }

  return { functionName, fileName, hostName, host, port, url, line, column };
};






function computeIsContentAndCategory(frame) {
  
  if (frame.category) {
    return;
  }

  let location = frame.location;

  
  
  
  
  for (let i = 0; i < location.length; i++) {
    if (location.charCodeAt(i) === CHAR_CODE_LPAREN) {
      if (isContentScheme(location, i + 1)) {
        frame.isContent = true;
        return;
      }

      for (let j = i + 1; j < location.length; j++) {
        if (location.charCodeAt(j) === CHAR_CODE_R &&
            isChromeScheme(location, j) &&
            (location.indexOf("resource://gre/modules/devtools") !== -1 ||
             location.indexOf("resource:///modules/devtools") !== -1)) {
          frame.category = global.CATEGORY_DEVTOOLS;
          return;
        }
      }

      break;
    }
  }

  
  if (isContentScheme(location, 0)) {
    frame.isContent = true;
    return;
  }

  if (location === "EnterJIT") {
    frame.category = global.CATEGORY_JIT;
    return;
  }

  frame.category = global.CATEGORY_OTHER;
}








function getInflatedFrameCache(frameTable) {
  let inflatedCache = gInflatedFrameStore.get(frameTable);
  if (inflatedCache !== undefined) {
    return inflatedCache;
  }

  
  inflatedCache = Array.from({ length: frameTable.data.length }, () => null);
  gInflatedFrameStore.set(frameTable, inflatedCache);
  return inflatedCache;
};










function getOrAddInflatedFrame(cache, index, frameTable, stringTable, allocationsTable) {
  let inflatedFrame = cache[index];
  if (inflatedFrame === null) {
    inflatedFrame = cache[index] = new InflatedFrame(index, frameTable, stringTable, allocationsTable);
  }
  return inflatedFrame;
};









function InflatedFrame(index, frameTable, stringTable, allocationsTable) {
  const LOCATION_SLOT = frameTable.schema.location;
  const IMPLEMENTATION_SLOT = frameTable.schema.implementation;
  const OPTIMIZATIONS_SLOT = frameTable.schema.optimizations;
  const LINE_SLOT = frameTable.schema.line;
  const CATEGORY_SLOT = frameTable.schema.category;

  let frame = frameTable.data[index];
  let category = frame[CATEGORY_SLOT];
  this.location = stringTable[frame[LOCATION_SLOT]];
  this.implementation = frame[IMPLEMENTATION_SLOT];
  this.optimizations = frame[OPTIMIZATIONS_SLOT];
  this.line = frame[LINE_SLOT];
  this.column = undefined;
  this.allocations = allocationsTable ? allocationsTable[index] : 0;
  this.category = category;
  this.isContent = false;

  
  
  
  
  
  
  computeIsContentAndCategory(this);
};










InflatedFrame.prototype.getFrameKey = function getFrameKey(options) {
  if (this.isContent || !options.contentOnly || options.isRoot) {
    options.isMetaCategoryOut = false;
    return this.location;
  }

  if (options.isLeaf) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    options.isMetaCategoryOut = true;
    return this.category;
  }

  
  return "";
};




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
  return isChromeScheme(url, 0) ? null : hostName;
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
      if (location.charCodeAt(i + 1) === CHAR_CODE_S) {
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

exports.computeIsContentAndCategory = computeIsContentAndCategory;
exports.parseLocation = parseLocation;
exports.getInflatedFrameCache = getInflatedFrameCache;
exports.getOrAddInflatedFrame = getOrAddInflatedFrame;
exports.InflatedFrame = InflatedFrame;
