


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);
loader.lazyRequireGetter(this, "CATEGORY_MAPPINGS",
  "devtools/profiler/global", true);
loader.lazyRequireGetter(this, "CATEGORY_JIT",
  "devtools/profiler/global", true);

const CHROME_SCHEMES = ["chrome://", "resource://"];
const CONTENT_SCHEMES = ["http://", "https://", "file://"];

exports.ThreadNode = ThreadNode;
exports.FrameNode = FrameNode;
exports._isContent = isContent; 
































function ThreadNode(threadSamples, contentOnly, beginAt, endAt) {
  this.samples = 0;
  this.duration = 0;
  this.calls = {};
  this._previousSampleTime = 0;

  for (let sample of threadSamples) {
    this.insert(sample, contentOnly, beginAt, endAt);
  }
}

ThreadNode.prototype = {
  













  insert: function(sample, contentOnly = false, beginAt = 0, endAt = Infinity) {
    let sampleTime = sample.time;
    if (!sampleTime || sampleTime < beginAt || sampleTime > endAt) {
      return;
    }

    let sampleFrames = sample.frames;
    let rootIndex = 1;

    
    
    if (contentOnly) {
      sampleFrames = sampleFrames.filter(frame => isContent(frame));
      rootIndex = 0;
    }
    if (!sampleFrames.length) {
      return;
    }

    let sampleDuration = sampleTime - this._previousSampleTime;
    this._previousSampleTime = sampleTime;
    this.samples++;
    this.duration += sampleDuration;

    FrameNode.prototype.insert(
      sampleFrames, rootIndex, sampleTime, sampleDuration, this.calls);
  },

  



  getInfo: function() {
    return {
      nodeType: "Thread",
      functionName: L10N.getStr("table.root"),
      categoryData: {}
    };
  }
};












function FrameNode({ location, line, category }) {
  this.location = location;
  this.line = line;
  this.category = category;
  this.sampleTimes = [];
  this.samples = 0;
  this.duration = 0;
  this.calls = {};
}

FrameNode.prototype = {
  


















  insert: function(frames, index, time, duration, _store = this.calls) {
    let frame = frames[index];
    if (!frame) {
      return;
    }
    let location = frame.location;
    let child = _store[location] || (_store[location] = new FrameNode(frame));
    child.sampleTimes.push({ start: time, end: time + duration });
    child.samples++;
    child.duration += duration;
    child.insert(frames, ++index, time, duration);
  },

  







  getInfo: function() {
    
    if (this.location == "EnterJIT") {
      this.category = CATEGORY_JIT;
    }

    
    
    let categoryData = CATEGORY_MAPPINGS[this.category] || {};

    
    let firstParen = this.location.indexOf("(");
    let lastColon = this.location.lastIndexOf(":");
    let resource = this.location.substring(firstParen + 1, lastColon);
    let line = this.location.substring(lastColon + 1).replace(")", "");
    let url = resource.split(" -> ").pop();
    let uri = nsIURL(url);
    let functionName, fileName, hostName;

    
    if (uri) {
      functionName = this.location.substring(0, firstParen - 1);
      fileName = (uri.fileName + (uri.ref ? "#" + uri.ref : "")) || "/";
      hostName = uri.host;
    } else {
      functionName = this.location;
      url = null;
      line = null;
    }

    return {
      nodeType: "Frame",
      functionName: functionName,
      fileName: fileName,
      hostName: hostName,
      url: url,
      line: line || this.line,
      categoryData: categoryData,
      isContent: !!isContent(this)
    };
  }
};









function isContent({ category, location }) {
  
  return !category &&
    !CHROME_SCHEMES.find(e => location.contains(e)) &&
    CONTENT_SCHEMES.find(e => location.contains(e));
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


let gNSURLStore = new Map();
