


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
  let plottedMinMaxSum = getMinMaxSum(plottedData);
  let sparsifiedData = sparsifyLineData(plottedData, plottedMinMaxSum, args);

  let response = { id, plottedData: sparsifiedData, plottedMinMaxSum };
  self.postMessage(response);
}






function getMinMaxSum(source) {
  let totalTicks = source.length;
  let maxValue = Number.MIN_SAFE_INTEGER;
  let minValue = Number.MAX_SAFE_INTEGER;
  let avgValue = 0;
  let sumValues = 0;

  for (let { value } of source) {
    maxValue = Math.max(value, maxValue);
    minValue = Math.min(value, minValue);
    sumValues += value;
  }
  avgValue = sumValues / totalTicks;

  return { minValue, maxValue, avgValue };
}





function sparsifyLineData(plottedData, plottedMinMaxSum, options) {
  let { width: graphWidth, height: graphHeight } = options;
  let { dataOffsetX, dampenValuesFactor } = options;
  let { minSquaredDistanceBetweenPoints } = options;

  let result = [];

  let totalTicks = plottedData.length;
  let maxValue = plottedMinMaxSum.maxValue;

  let firstTick = totalTicks ? plottedData[0].delta : 0;
  let lastTick = totalTicks ? plottedData[totalTicks - 1].delta : 0;
  let dataScaleX = graphWidth / (lastTick - dataOffsetX);
  let dataScaleY = graphHeight / maxValue * dampenValuesFactor;

  let prevX = 0;
  let prevY = 0;

  for (let { delta, value } of plottedData) {
    let currX = (delta - dataOffsetX) * dataScaleX;
    let currY = graphHeight - value * dataScaleY;

    if (delta == firstTick || delta == lastTick) {
      result.push({ delta, value });
      continue;
    }

    let dist = distSquared(prevX, prevY, currX, currY);
    if (dist >= minSquaredDistanceBetweenPoints) {
      result.push({ delta, value });
      prevX = currX;
      prevY = currY;
    }
  }

  return result;
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




function distSquared(x0, y0, x1, y1) {
  let xs = x1 - x0;
  let ys = y1 - y0;
  return xs * xs + ys * ys;
}
