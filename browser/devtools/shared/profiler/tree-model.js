


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const {extend} = require("sdk/util/object");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/profiler/global", true);
loader.lazyRequireGetter(this, "CATEGORY_MAPPINGS",
  "devtools/shared/profiler/global", true);
loader.lazyRequireGetter(this, "CATEGORIES",
  "devtools/shared/profiler/global", true);
loader.lazyRequireGetter(this, "CATEGORY_JIT",
  "devtools/shared/profiler/global", true);
loader.lazyRequireGetter(this, "JITOptimizations",
  "devtools/shared/profiler/jit", true);
loader.lazyRequireGetter(this, "CATEGORY_OTHER",
  "devtools/shared/profiler/global", true);

const CHROME_SCHEMES = ["chrome://", "resource://", "jar:file://"];
const CONTENT_SCHEMES = ["http://", "https://", "file://", "app://"];

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
    let optimizations = options.optimizations;
    let sampleTime = sample.time;
    if (!sampleTime || sampleTime < startTime || sampleTime > endTime) {
      return;
    }

    let sampleFrames = sample.frames;

    
    
    if (options.contentOnly) {
      
      sampleFrames = filterPlatformData(sampleFrames);
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
      sampleFrames, optimizations, 0, sampleTime, sampleDuration, this.calls);
  },

  



  getInfo: function() {
    return {
      nodeType: "Thread",
      functionName: L10N.getStr("table.root"),
      categoryData: {}
    };
  },

  








  hasOptimizations: function () {
    return null;
  }
};



















function FrameNode({ location, line, column, category, allocations, isMetaCategory }) {
  this.location = location;
  this.line = line;
  this.column = column;
  this.category = category;
  this.allocations = allocations || 0;
  this.sampleTimes = [];
  this.samples = 0;
  this.duration = 0;
  this.calls = {};
  this._optimizations = null;
  this.isMetaCategory = isMetaCategory;
}

FrameNode.prototype = {
  




















  insert: function(frames, optimizations, index, time, duration, _store = this.calls) {
    let frame = frames[index];
    if (!frame) {
      return;
    }
    
    
    
    
    let key = frame.isMetaCategory ? frame.category : frame.location;
    let child = _store[key] || (_store[key] = new FrameNode(frame));
    child.sampleTimes.push({ start: time, end: time + duration });
    child.samples++;
    child.duration += duration;
    if (optimizations && frame.optsIndex != null) {
      let opts = child._optimizations || (child._optimizations = new JITOptimizations(optimizations));
      opts.addOptimizationSite(frame.optsIndex);
    }
    child.insert(frames, optimizations, index + 1, time, duration);
  },

  







  getInfo: function() {
    return this._data || this._computeInfo();
  },

  



  _computeInfo: function() {
    
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
      hostName = url.indexOf("jar:") == 0 ? "" : uri.host;
    } else {
      functionName = this.location;
      url = null;
    }

    return this._data = {
      nodeType: "Frame",
      functionName: functionName,
      fileName: fileName,
      hostName: hostName,
      url: url,
      line: line,
      column: column,
      categoryData: categoryData,
      isContent: !!isContent(this),
      isMetaCategory: this.isMetaCategory
    };
  },

  




  hasOptimizations: function () {
    return !!this._optimizations;
  },

  





  getOptimizations: function () {
    return this._optimizations;
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
























function filterPlatformData (frames) {
  let result = [];
  let last = frames.length - 1;
  let frame;

  for (let i = 0; i < frames.length; i++) {
    frame = frames[i];
    if (isContent(frame)) {
      result.push(frame);
    } else if (last === i) {
      
      
      
      
      result.push(extend({ isMetaCategory: true, category: CATEGORY_OTHER }, frame));
    }
  }

  return result;
}
