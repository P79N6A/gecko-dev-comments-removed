


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "CATEGORY_MAPPINGS",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "JITOptimizations",
  "devtools/performance/jit", true);
loader.lazyRequireGetter(this, "FrameUtils",
  "devtools/performance/frame-utils");

















function ThreadNode(thread, options = {}) {
  if (options.endTime == void 0 || options.startTime == void 0) {
    throw new Error("ThreadNode requires both `startTime` and `endTime`.");
  }
  this.samples = 0;
  this.sampleTimes = [];
  this.youngestFrameSamples = 0;
  this.calls = [];
  this.duration = options.endTime - options.startTime;

  let { samples, stackTable, frameTable, stringTable, allocationsTable } = thread;

  
  if (samples.data.length === 0) {
    return;
  }

  this._buildInverted(samples, stackTable, frameTable, stringTable, allocationsTable, options);
  if (!options.invertTree) {
    this._uninvert();
  }
}

ThreadNode.prototype = {
  

























  _buildInverted: function buildInverted(samples, stackTable, frameTable, stringTable, allocationsTable, options) {
    function getOrAddFrameNode(calls, isLeaf, frameKey, inflatedFrame, isMetaCategory, leafTable) {
      
      let frameNode;

      
      
      
      
      
      if (isLeaf) {
        frameNode = leafTable[frameKey];
      } else {
        for (let i = 0; i < calls.length; i++) {
          if (calls[i].key === frameKey) {
            frameNode = calls[i];
            break;
          }
        }
      }

      if (!frameNode) {
        frameNode = new FrameNode(frameKey, inflatedFrame, isMetaCategory);
        if (isLeaf) {
          leafTable[frameKey] = frameNode;
        }
        calls.push(frameNode);
      }

      return frameNode;
    }

    const SAMPLE_STACK_SLOT = samples.schema.stack;
    const SAMPLE_TIME_SLOT = samples.schema.time;

    const STACK_PREFIX_SLOT = stackTable.schema.prefix;
    const STACK_FRAME_SLOT = stackTable.schema.frame;

    const InflatedFrame = FrameUtils.InflatedFrame;
    const getOrAddInflatedFrame = FrameUtils.getOrAddInflatedFrame;

    let samplesData = samples.data;
    let stacksData = stackTable.data;

    
    let inflatedFrameCache = FrameUtils.getInflatedFrameCache(frameTable);
    let leafTable = Object.create(null);

    let startTime = options.startTime;
    let endTime = options.endTime;
    let flattenRecursion = options.flattenRecursion;

    
    let mutableFrameKeyOptions = {
      contentOnly: options.contentOnly,
      isRoot: false,
      isLeaf: false,
      isMetaCategoryOut: false
    };

    for (let i = 0; i < samplesData.length; i++) {
      let sample = samplesData[i];
      let sampleTime = sample[SAMPLE_TIME_SLOT];

      
      
      
      
      
      if (!sampleTime || sampleTime <= startTime || sampleTime > endTime) {
        continue;
      }

      let stackIndex = sample[SAMPLE_STACK_SLOT];
      let calls = this.calls;
      let prevCalls = this.calls;
      let prevFrameKey;
      let isLeaf = mutableFrameKeyOptions.isLeaf = true;
      let skipRoot = options.invertTree;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      while (stackIndex !== null) {
        let stackEntry = stacksData[stackIndex];
        let frameIndex = stackEntry[STACK_FRAME_SLOT];

        
        stackIndex = stackEntry[STACK_PREFIX_SLOT];

        
        
        
        if (stackIndex === null && skipRoot) {
          break;
        }

        
        let inflatedFrame = getOrAddInflatedFrame(inflatedFrameCache, frameIndex, frameTable,
                                                  stringTable, allocationsTable);

        
        mutableFrameKeyOptions.isRoot = stackIndex === null;
        let frameKey = inflatedFrame.getFrameKey(mutableFrameKeyOptions);

        
        if (frameKey === "") {
          continue;
        }

        
        
        if (!flattenRecursion || frameKey !== prevFrameKey) {
          calls = prevCalls;
        }

        let frameNode = getOrAddFrameNode(calls, isLeaf, frameKey, inflatedFrame,
                                          mutableFrameKeyOptions.isMetaCategoryOut,
                                          leafTable);
        if (isLeaf) {
          frameNode.youngestFrameSamples++;
          if (inflatedFrame.optimizations) {
            frameNode._addOptimizations(inflatedFrame.optimizations, inflatedFrame.implementation,
                                        sampleTime, stringTable);
          }
        }
        frameNode.samples++;

        prevFrameKey = frameKey;
        prevCalls = frameNode.calls;
        isLeaf = mutableFrameKeyOptions.isLeaf = false;
      }

      this.samples++;
      this.sampleTimes.push(sampleTime);
    }
  },

  


  _uninvert: function uninvert() {
    function mergeOrAddFrameNode(calls, node) {
      
      
      
      for (let i = 0; i < calls.length; i++) {
        if (calls[i].key === node.key) {
          let foundNode = calls[i];
          foundNode._merge(node);
          return foundNode.calls;
        }
      }
      let copy = node._clone();
      calls.push(copy);
      return copy.calls;
    }

    let workstack = [{ node: this, level: 0 }];
    let spine = [];
    let entry;

    
    let rootCalls = [];

    
    while (entry = workstack.pop()) {
      spine[entry.level] = entry;

      let node = entry.node;
      let calls = node.calls;

      if (calls.length === 0) {
        
        
        let uninvertedCalls = rootCalls;
        for (let level = entry.level; level > 0; level--) {
          let callee = spine[level];
          uninvertedCalls = mergeOrAddFrameNode(uninvertedCalls, callee.node);
        }
      } else {
        
        for (let i = 0; i < calls.length; i++) {
          workstack.push({ node: calls[i], level: entry.level + 1 });
        }
      }
    }

    
    
    this.calls = rootCalls;
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


































function FrameNode(frameKey, { location, line, category, allocations, isContent }, isMetaCategory) {
  this.key = frameKey;
  this.location = location;
  this.line = line;
  this.allocations = allocations;
  this.youngestFrameSamples = 0;
  this.samples = 0;
  this.calls = [];
  this.isContent = !!isContent;
  this._optimizations = null;
  this._tierData = null;
  this._stringTable = null;
  this.isMetaCategory = !!isMetaCategory;
  this.category = category;
}

FrameNode.prototype = {
  













  _addOptimizations: function (site, implementation, time, stringTable) {
    
    
    if (site) {
      let opts = this._optimizations;
      if (opts === null) {
        opts = this._optimizations = [];
        this._stringTable = stringTable;
      }
      opts.push(site);

      if (this._tierData === null) {
        this._tierData = [];
      }
      
      this._tierData.push({ implementation, time });
    }
  },

  _clone: function () {
    let newNode = new FrameNode(this.key, this, this.isMetaCategory);
    newNode._merge(this);
    return newNode;
  },

  _merge: function (otherNode) {
    if (this === otherNode) {
      return;
    }

    this.samples += otherNode.samples;
    this.youngestFrameSamples += otherNode.youngestFrameSamples;

    if (otherNode._optimizations) {
      let opts = this._optimizations;
      if (opts === null) {
        opts = this._optimizations = [];
        this._stringTable = otherNode._stringTable;
      }
      let otherOpts = otherNode._optimizations;
      for (let i = 0; i < otherOpts.length; i++) {
        opts.push(otherOpts[i]);
      }
    }
  },

  







  getInfo: function() {
    return this._data || this._computeInfo();
  },

  



  _computeInfo: function() {
    let categoryData = CATEGORY_MAPPINGS[this.category] || {};
    let parsedData = FrameUtils.parseLocation(this.location, this.line, this.column);
    parsedData.nodeType = "Frame";
    parsedData.categoryData = categoryData;
    parsedData.isContent = this.isContent;
    parsedData.isMetaCategory = this.isMetaCategory;

    return this._data = parsedData;
  },

  




  hasOptimizations: function () {
    return !!this._optimizations;
  },

  





  getOptimizations: function () {
    if (!this._optimizations) {
      return null;
    }
    return new JITOptimizations(this._optimizations, this._stringTable);
  },

  




  getOptimizationTierData: function () {
    if (!this._tierData) {
      return null;
    }
    return this._tierData;
  }
};

exports.ThreadNode = ThreadNode;
exports.FrameNode = FrameNode;
