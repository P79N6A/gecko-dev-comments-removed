


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "CATEGORY_MAPPINGS",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "CATEGORIES",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "CATEGORY_JIT",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "CATEGORY_OTHER",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "JITOptimizations",
  "devtools/performance/jit", true);
loader.lazyRequireGetter(this, "FrameUtils",
  "devtools/performance/frame-utils");

















function ThreadNode(thread, options = {}) {
  this.samples = 0;
  this.duration = 0;
  this.calls = [];

  
  this.selfCount = Object.create(null);
  this.selfDuration = Object.create(null);

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

    let selfCount = this.selfCount;
    let selfDuration = this.selfDuration;

    let samplesData = samples.data;
    let stacksData = stackTable.data;

    
    let inflatedFrameCache = FrameUtils.getInflatedFrameCache(frameTable);
    let leafTable = Object.create(null);

    let startTime = options.startTime || 0
    let endTime = options.endTime || Infinity;
    let flattenRecursion = options.flattenRecursion;

    
    
    
    let prevSampleTime = samplesData[0][SAMPLE_TIME_SLOT];

    
    let mutableFrameKeyOptions = {
      contentOnly: options.contentOnly,
      isRoot: false,
      isLeaf: false,
      isMetaCategoryOut: false
    };

    
    
    for (let i = 1; i < samplesData.length; i++) {
      let sample = samplesData[i];
      let sampleTime = sample[SAMPLE_TIME_SLOT];

      
      
      
      
      
      if (!sampleTime || sampleTime <= startTime || sampleTime > endTime) {
        prevSampleTime = sampleTime;
        continue;
      }

      let sampleDuration = sampleTime - prevSampleTime;
      let stackIndex = sample[SAMPLE_STACK_SLOT];
      let calls = this.calls;
      let prevCalls = this.calls;
      let prevFrameKey;
      let isLeaf = mutableFrameKeyOptions.isLeaf = true;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      while (stackIndex !== null) {
        let stackEntry = stacksData[stackIndex];
        let frameIndex = stackEntry[STACK_FRAME_SLOT];

        
        stackIndex = stackEntry[STACK_PREFIX_SLOT];

        
        let inflatedFrame = getOrAddInflatedFrame(inflatedFrameCache, frameIndex, frameTable,
                                                  stringTable, allocationsTable);

        
        mutableFrameKeyOptions.isRoot = stackIndex === null;
        let frameKey = inflatedFrame.getFrameKey(mutableFrameKeyOptions);

        
        
        if (isLeaf) {
          
          
          if (selfCount[frameKey] === undefined) {
            selfCount[frameKey] = 0;
            selfDuration[frameKey] = 0;
          }
          selfCount[frameKey]++;
          selfDuration[frameKey] += sampleDuration;
        } else {
          
          if (frameKey === "") {
            continue;
          }
        }

        
        
        if (!flattenRecursion || frameKey !== prevFrameKey) {
          calls = prevCalls;
        }

        let frameNode = getOrAddFrameNode(calls, isLeaf, frameKey, inflatedFrame,
                                          mutableFrameKeyOptions.isMetaCategoryOut,
                                          leafTable);

        frameNode._countSample(prevSampleTime, sampleTime, inflatedFrame.optimizations,
                               stringTable);

        prevFrameKey = frameKey;
        prevCalls = frameNode.calls;
        isLeaf = mutableFrameKeyOptions.isLeaf = false;
      }

      this.duration += sampleDuration;
      this.samples++;
      prevSampleTime = sampleTime;
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
  this.category = category;
  this.allocations = allocations;
  this.samples = 0;
  this.duration = 0;
  this.calls = [];
  this.isContent = isContent;
  this._optimizations = null;
  this._stringTable = null;
  this.isMetaCategory = isMetaCategory;
}

FrameNode.prototype = {
  












  _countSample: function (prevSampleTime, sampleTime, optimizationSite, stringTable) {
    this.samples++;
    this.duration += sampleTime - prevSampleTime;

    
    
    if (optimizationSite) {
      let opts = this._optimizations;
      if (opts === null) {
        opts = this._optimizations = [];
        this._stringTable = stringTable;
      }
      opts.push(optimizationSite);
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
    this.duration += otherNode.duration;

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
    
    if (this.location == "EnterJIT") {
      this.category = CATEGORY_JIT;
    }

    if (this.isMetaCategory && !this.category) {
      this.category = CATEGORY_OTHER;
    }

    
    
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
  }
};

exports.ThreadNode = ThreadNode;
exports.FrameNode = FrameNode;
exports.FrameNode.isContent = FrameUtils.isContent;
