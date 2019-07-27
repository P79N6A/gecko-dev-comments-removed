


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

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
loader.lazyRequireGetter(this, "FrameUtils",
  "devtools/shared/profiler/frame-utils");

exports.ThreadNode = ThreadNode;
exports.FrameNode = FrameNode;
exports.FrameNode.isContent = FrameUtils.isContent;


































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

    if (!options.invertTree) {
      
      
      
      
      sampleFrames = sampleFrames.slice(1);
    }

    
    
    if (options.contentOnly) {
      sampleFrames = FrameUtils.filterPlatformData(sampleFrames);
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

    let parsedData = FrameUtils.parseLocation(this);
    parsedData.nodeType = "Frame";
    parsedData.categoryData = categoryData;
    parsedData.isContent = FrameUtils.isContent(this);
    parsedData.isMetaCategory = this.isMetaCategory;

    return this._data = parsedData;
  },

  




  hasOptimizations: function () {
    return !!this._optimizations;
  },

  





  getOptimizations: function () {
    return this._optimizations;
  }
};
