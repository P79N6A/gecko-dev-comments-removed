


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
exports.FrameNode.isContent = isContent;
































function ThreadNode(threadSamples, options = {}) {
  this.samples = 0;
  this.duration = 0;
  this.calls = {};
  this._previousSampleTime = 0;

  for (let sample of threadSamples) {
    this.insert(sample, options);
  }
}

ThreadNode.prototype = {
  













  insert: function(sample, options = {}) {
    let startTime = options.startTime || 0;
    let endTime = options.endTime || Infinity;
    let sampleTime = sample.time;
    if (!sampleTime || sampleTime < startTime || sampleTime > endTime) {
      return;
    }

    let sampleFrames = sample.frames;

    
    
    if (options.contentOnly) {
      
      sampleFrames = sampleFrames.filter(isContent);
    } else {
      
      sampleFrames = sampleFrames.slice(1);
    }
    
    
    if (!sampleFrames.length) {
      return;
    }
    
    if (options.invertTree) {
      sampleFrames.reverse();
    }

    let sampleDuration = sampleTime - this._previousSampleTime;
    this._previousSampleTime = sampleTime;
    this.samples++;
    this.duration += sampleDuration;

    FrameNode.prototype.insert(
      sampleFrames, 0, sampleTime, sampleDuration, this.calls);
  },

  



  getInfo: function() {
    return {
      nodeType: "Thread",
      functionName: L10N.getStr("table.root"),
      categoryData: {}
    };
  }
};














function FrameNode({ location, line, column, category }) {
  this.location = location;
  this.line = line;
  this.column = column;
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

    
    let lineAndColumn = this.location.match(/((:\d+)*)\)?$/)[1];
    let [, line, column] = lineAndColumn.split(":");
    line = line || this.line;
    column = column || this.column;

    let firstParenIndex = this.location.indexOf("(");
    let lineAndColumnIndex = this.location.indexOf(lineAndColumn);
    let resource = this.location.substring(firstParenIndex + 1, lineAndColumnIndex);

    let url = resource.split(" -> ").pop();
    let uri = nsIURL(url);
    let functionName, fileName, hostName;

    
    if (uri) {
      functionName = this.location.substring(0, firstParenIndex - 1);
      fileName = (uri.fileName + (uri.ref ? "#" + uri.ref : "")) || "/";
      hostName = uri.host;
    } else {
      functionName = this.location;
      url = null;
    }

    return {
      nodeType: "Frame",
      functionName: functionName,
      fileName: fileName,
      hostName: hostName,
      url: url,
      line: line,
      column: column,
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
