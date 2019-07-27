


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
