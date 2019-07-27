


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");






exports.RecordingUtils = {};










exports.RecordingUtils.filterSamples = function(profile, profilerStartTime) {
  let firstThread = profile.threads[0];
  const TIME_SLOT = firstThread.samples.schema.time;
  firstThread.samples.data = firstThread.samples.data.filter(e => {
    return e[TIME_SLOT] >= profilerStartTime;
  });
}









exports.RecordingUtils.offsetSampleTimes = function(profile, timeOffset) {
  let firstThread = profile.threads[0];
  const TIME_SLOT = firstThread.samples.schema.time;
  let samplesData = firstThread.samples.data;
  for (let i = 0; i < samplesData.length; i++) {
    samplesData[i][TIME_SLOT] -= timeOffset;
  }
}









exports.RecordingUtils.offsetMarkerTimes = function(markers, timeOffset) {
  for (let marker of markers) {
    marker.start -= timeOffset;
    marker.end -= timeOffset;
  }
}












exports.RecordingUtils.offsetAndScaleTimestamps = function(timestamps, timeOffset, timeScale) {
  for (let i = 0, len = timestamps.length; i < len; i++) {
    timestamps[i] -= timeOffset;
    timestamps[i] /= timeScale;
  }
}




let gSamplesFromAllocationCache = new WeakMap();












exports.RecordingUtils.getSamplesFromAllocations = function(allocations) {
  let cached = gSamplesFromAllocationCache.get(allocations);
  if (cached) {
    return cached;
  }

  let { sites, timestamps, frames, counts } = allocations;
  let samples = [];

  for (let i = 0, len = sites.length; i < len; i++) {
    let site = sites[i];
    let timestamp = timestamps[i];
    let frame = frames[site];
    let count = counts[site];

    let sample = { time: timestamp, frames: [] };
    samples.push(sample);

    while (frame) {
      let source = frame.source + ":" + frame.line + ":" + frame.column;
      let funcName = frame.functionDisplayName || "";

      sample.frames.push({
        location: funcName ? funcName + " (" + source + ")" : source,
        allocations: count
      });

      site = frame.parent;
      frame = frames[site];
      count = counts[site];
    }

    sample.frames.reverse();
  }

  gSamplesFromAllocationCache.set(allocations, samples);
  return samples;
}











exports.RecordingUtils.getFilteredBlueprint = function({ blueprint, hiddenMarkers }) {
  let filteredBlueprint = Cu.cloneInto(blueprint, {});
  let maybeRemovedGroups = new Set();
  let removedGroups = new Set();

  

  for (let hiddenMarkerName of hiddenMarkers) {
    maybeRemovedGroups.add(filteredBlueprint[hiddenMarkerName].group);
    delete filteredBlueprint[hiddenMarkerName];
  }

  

  for (let maybeRemovedGroup of maybeRemovedGroups) {
    let markerNames = Object.keys(filteredBlueprint);
    let isGroupRemoved = markerNames.every(e => filteredBlueprint[e].group != maybeRemovedGroup);
    if (isGroupRemoved) {
      removedGroups.add(maybeRemovedGroup);
    }
  }

  

  for (let removedGroup of removedGroups) {
    let markerNames = Object.keys(filteredBlueprint);
    for (let markerName of markerNames) {
      let markerDetails = filteredBlueprint[markerName];
      if (markerDetails.group > removedGroup) {
        markerDetails.group--;
      }
    }
  }

  return filteredBlueprint;
};













exports.RecordingUtils.deflateProfile = function deflateProfile(profile) {
  profile.threads = profile.threads.map((thread) => {
    let uniqueStacks = new UniqueStacks();
    return deflateThread(thread, uniqueStacks);
  });

  profile.meta.version = 3;
};










function deflateStack(frames, uniqueStacks) {
  
  
  let prefixIndex = null;
  for (let i = 0; i < frames.length; i++) {
    let frameIndex = uniqueStacks.getOrAddFrameIndex(frames[i]);
    prefixIndex = uniqueStacks.getOrAddStackIndex(prefixIndex, frameIndex);
  }
  return prefixIndex;
}










function deflateSamples(samples, uniqueStacks) {
  
  

  let deflatedSamples = new Array(samples.length);
  for (let i = 0; i < samples.length; i++) {
    let sample = samples[i];
    deflatedSamples[i] = [
      deflateStack(sample.frames, uniqueStacks),
      sample.time,
      sample.responsiveness,
      sample.rss,
      sample.uss,
      sample.frameNumber,
      sample.power
    ];
  }

  let slot = 0;
  return {
    schema: {
      stack: slot++,
      time: slot++,
      responsiveness: slot++,
      rss: slot++,
      uss: slot++,
      frameNumber: slot++,
      power: slot++
    },
    data: deflatedSamples
  };
}













function deflateMarkers(markers, uniqueStacks) {
  
  

  let deflatedMarkers = new Array(markers.length);
  for (let i = 0; i < markers.length; i++) {
    let marker = markers[i];
    if (marker.data && marker.data.type === "tracing" && marker.data.stack) {
      marker.data.stack = deflateThread(marker.data.stack, uniqueStacks);
    }

    deflatedMarkers[i] = [
      uniqueStacks.getOrAddStringIndex(marker.name),
      marker.time,
      marker.data
    ];
  }

  let slot = 0;
  return {
    schema: {
      name: slot++,
      time: slot++,
      data: slot++
    },
    data: deflatedMarkers
  };
}









function deflateThread(thread, uniqueStacks) {
  return {
    name: thread.name,
    tid: thread.tid,
    samples: deflateSamples(thread.samples, uniqueStacks),
    markers: deflateMarkers(thread.markers, uniqueStacks),
    stackTable: uniqueStacks.getStackTableWithSchema(),
    frameTable: uniqueStacks.getFrameTableWithSchema(),
    stringTable: uniqueStacks.stringTable
  };
}













































function UniqueStacks() {
  this._frameTable = [];
  this._stackTable = [];
  this.stringTable = [];
  this._frameHash = Object.create(null);
  this._stackHash = Object.create(null);
  this._stringHash = Object.create(null);
}

UniqueStacks.prototype.getStackTableWithSchema = function() {
  let slot = 0;
  return {
    schema: {
      prefix: slot++,
      frame: slot++
    },
    data: this._stackTable
  };
};

UniqueStacks.prototype.getFrameTableWithSchema = function() {
  let slot = 0;
  return {
    schema: {
      location: slot++,
      implementation: slot++,
      optimizations: slot++,
      line: slot++,
      category: slot++
    },
    data: this._frameTable
  };
}

UniqueStacks.prototype.getOrAddFrameIndex = function(frame) {
  
  

  let frameHash = this._frameHash;
  let frameTable = this._frameTable;

  let locationIndex = this.getOrAddStringIndex(frame.location);
  let implementationIndex = this.getOrAddStringIndex(frame.implementation);

  
  let hash = `${locationIndex} ${implementationIndex || ""} ${frame.line || ""} ${frame.category || ""}`;

  let index = frameHash[hash];
  if (index !== undefined) {
    return index;
  }

  index = frameTable.length;
  frameHash[hash] = index;
  frameTable.push([
    this.getOrAddStringIndex(frame.location),
    this.getOrAddStringIndex(frame.implementation),
    
    
    null,
    frame.line,
    frame.category
  ]);
  return index;
};

UniqueStacks.prototype.getOrAddStackIndex = function(prefixIndex, frameIndex) {
  
  

  let stackHash = this._stackHash;
  let stackTable = this._stackTable;

  
  let hash = prefixIndex + " " + frameIndex;

  let index = stackHash[hash];
  if (index !== undefined) {
    return index;
  }

  index = stackTable.length;
  stackHash[hash] = index;
  stackTable.push([prefixIndex, frameIndex]);
  return index;
};

UniqueStacks.prototype.getOrAddStringIndex = function(s) {
  if (!s) {
    return null;
  }

  let stringHash = this._stringHash;
  let stringTable = this.stringTable;
  let index = stringHash[s];
  if (index !== undefined) {
    return index;
  }

  index = stringTable.length;
  stringHash[s] = index;
  stringTable.push(s);
  return index;
};
