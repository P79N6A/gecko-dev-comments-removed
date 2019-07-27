



"use strict";

let {Class} = require("sdk/core/heritage");







let StackFrameCache = Class({
  


  initialize: function() {
    this._framesToCounts = null;
    this._framesToIndices = null;
    this._framesToForms = null;
    this._lastEventSize = 0;
  },

  


  initFrames: function() {
    if (this._framesToCounts) {
      
      return;
    }

    this._framesToCounts = new Map();
    this._framesToIndices = new Map();
    this._framesToForms = new Map();
    this._lastEventSize = 0;
  },

  


  clearFrames: function() {
    this._framesToCounts.clear();
    this._framesToCounts = null;
    this._framesToIndices.clear();
    this._framesToIndices = null;
    this._framesToForms.clear();
    this._framesToForms = null;
    this._lastEventSize = 0;
  },

  



  addFrame: function(frame) {
    this._assignFrameIndices(frame);
    this._createFrameForms(frame);
    this._countFrame(frame);
    return this._framesToIndices.get(frame);
  },

  











  updateFramePacket: function(packet) {
    
    
    
    const size = this._framesToForms.size;
    packet.frames = Array(size).fill(null);
    packet.counts = Array(size).fill(0);

    
    for (let [stack, index] of this._framesToIndices) {
      packet.frames[index] = this._framesToForms.get(stack);
      packet.counts[index] = this._framesToCounts.get(stack) || 0;
    }

    return packet;
  },

  



























  makeEvent: function() {
    const size = this._framesToForms.size;
    if (!size || size <= this._lastEventSize) {
      return null;
    }

    let packet = Array(size - this._lastEventSize).fill(null);
    for (let [stack, index] of this._framesToIndices) {
      if (index >= this._lastEventSize) {
        packet[index - this._lastEventSize] = this._framesToForms.get(stack);
      }
    }

    this._lastEventSize = size;

    return packet;
  },

  






  _assignFrameIndices: function(frame) {
    if (this._framesToIndices.has(frame)) {
      return;
    }

    if (frame) {
      this._assignFrameIndices(frame.parent);
    }

    const index = this._framesToIndices.size;
    this._framesToIndices.set(frame, index);
  },

  





  _createFrameForms: function(frame) {
    if (this._framesToForms.has(frame)) {
      return;
    }

    let form = null;
    if (frame) {
      form = {
        line: frame.line,
        column: frame.column,
        source: frame.source,
        functionDisplayName: frame.functionDisplayName,
        parent: this._framesToIndices.get(frame.parent)
      };
      this._createFrameForms(frame.parent);
    }

    this._framesToForms.set(frame, form);
  },

  





  _countFrame: function(frame) {
    if (!this._framesToCounts.has(frame)) {
      this._framesToCounts.set(frame, 1);
    } else {
      let count = this._framesToCounts.get(frame);
      this._framesToCounts.set(frame, count + 1);
    }
  }
});

exports.StackFrameCache = StackFrameCache;
