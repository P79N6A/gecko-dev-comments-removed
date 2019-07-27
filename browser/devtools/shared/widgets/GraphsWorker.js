


"use strict";

self.onmessage = e => {
  const { id, task, args } = e.data;

  switch (task) {
    case "plotTimestampsGraph":
      plotTimestampsGraph(id, args);
      break;
    default:
      self.postMessage({ id, error: e.message + "\n" + e.stack });
      break;
  }
};








function plotTimestampsGraph(id, args) {
  let plottedData = plotTimestamps(args.timestamps, args.interval);
  let plottedMinMaxSum = getMinMaxAvg(plottedData, args.timestamps, args.duration);

  let response = { id, plottedData, plottedMinMaxSum };
  self.postMessage(response);
}








function getMinMaxAvg(source, timestamps, duration) {
  let totalTicks = source.length;
  let totalFrames = timestamps.length;
  let maxValue = Number.MIN_SAFE_INTEGER;
  let minValue = Number.MAX_SAFE_INTEGER;
  
  
  
  
  let avgValue = totalFrames / (duration / 1000);

  for (let { value } of source) {
    maxValue = Math.max(value, maxValue);
    minValue = Math.min(value, minValue);
  }

  return { minValue, maxValue, avgValue };
}

















function plotTimestamps(timestamps, interval = 100, clamp = 60) {
  let timeline = [];
  let totalTicks = timestamps.length;

  
  
  if (totalTicks == 0) {
    timeline.push({ delta: 0, value: 0 });
    timeline.push({ delta: interval, value: 0 });
    return timeline;
  }

  let frameCount = 0;
  let prevTime = +timestamps[0];

  for (let i = 1; i < totalTicks; i++) {
    let currTime = +timestamps[i];
    frameCount++;

    let elapsedTime = currTime - prevTime;
    if (elapsedTime < interval) {
      continue;
    }

    let rate = Math.min(1000 / (elapsedTime / frameCount), clamp);
    timeline.push({ delta: prevTime, value: rate });
    timeline.push({ delta: currTime, value: rate });

    frameCount = 0;
    prevTime = currTime;
  }

  return timeline;
}
