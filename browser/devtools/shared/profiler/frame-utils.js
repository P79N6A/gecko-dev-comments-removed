


"use strict";

const { Ci } = require("chrome");
const { extend } = require("sdk/util/object");
loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "CATEGORY_OTHER",
  "devtools/shared/profiler/global", true);


const gNSURLStore = new Map();
const CHROME_SCHEMES = ["chrome://", "resource://", "jar:file://"];
const CONTENT_SCHEMES = ["http://", "https://", "file://", "app://"];





exports.parseLocation = function parseLocation (frame) {
  
  let lineAndColumn = frame.location.match(/((:\d+)*)\)?$/)[1];
  let [, line, column] = lineAndColumn.split(":");
  line = line || frame.line;
  column = column || frame.column;

  let firstParenIndex = frame.location.indexOf("(");
  let lineAndColumnIndex = frame.location.indexOf(lineAndColumn);
  let resource = frame.location.substring(firstParenIndex + 1, lineAndColumnIndex);

  let url = resource.split(" -> ").pop();
  let uri = nsIURL(url);
  let functionName, fileName, hostName;

  
  if (uri) {
    functionName = frame.location.substring(0, firstParenIndex - 1);
    fileName = (uri.fileName + (uri.ref ? "#" + uri.ref : "")) || "/";
    hostName = getHost(url, uri.host);
  } else {
    functionName = frame.location;
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
},









exports.isContent = function isContent ({ category, location }) {
  
  return !!(!category &&
    !CHROME_SCHEMES.find(e => location.contains(e)) &&
    CONTENT_SCHEMES.find(e => location.contains(e)));
}
























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
  if (cached) {
    return cached;
  }
  let uri = null;
  try {
    uri = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
  } catch(e) {
    
  }
  gNSURLStore.set(url, uri);
  return uri;
}





function getHost (url, hostName) {
  if (CHROME_SCHEMES.find(e => url.indexOf(e) === 0)) {
    return null;
  }
  return hostName;
}
