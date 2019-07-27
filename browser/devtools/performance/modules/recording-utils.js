


"use strict";






exports.RecordingUtils = {};










exports.RecordingUtils.filterSamples = function(profile, profilerStartTime) {
  let firstThread = profile.threads[0];

  firstThread.samples = firstThread.samples.filter(e => {
    return e.time >= profilerStartTime;
  });
}









exports.RecordingUtils.offsetSampleTimes = function(profile, timeOffset) {
  let firstThread = profile.threads[0];

  for (let sample of firstThread.samples) {
    sample.time -= timeOffset;
  }
}









exports.RecordingUtils.offsetMarkerTimes = function(markers, timeOffset) {
  for (let marker of markers) {
    marker.start -= timeOffset;
    marker.end -= timeOffset;
  }
}












exports.RecordingUtils.getSamplesFromAllocations = function(allocations) {
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
      sample.frames.push({
        location: frame.source + ":" + frame.line + ":" + frame.column,
        allocations: count
      });
      site = frame.parent;
      frame = frames[site];
      count = counts[site];
    }

    sample.frames.reverse();
  }

  return samples;
}
