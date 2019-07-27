





"use strict";



const OPTIMAL_TIME_INTERVAL_MAX_ITERS = 100;

const TIME_INTERVAL_MULTIPLE = 10;
const TIME_INTERVAL_SCALES = 3;

const TIME_GRADUATION_MIN_SPACING = 10;

const TIME_INTERVAL_COLOR = [128, 136, 144];
const TIME_INTERVAL_OPACITY_MIN = 32; 
const TIME_INTERVAL_OPACITY_ADD = 32; 











function createNode(options) {
  if (!options.parent) {
    throw new Error("Missing parent DOMNode to create new node");
  }

  let type = options.nodeType || "div";
  let node = options.parent.ownerDocument.createElement(type);

  for (let name in options.attributes || {}) {
    let value = options.attributes[name];
    node.setAttribute(name, value);
  }

  if (options.textContent) {
    node.textContent = options.textContent;
  }

  options.parent.appendChild(node);
  return node;
}

exports.createNode = createNode;










function drawGraphElementBackground(document, id, graphWidth, timeScale) {
  let canvas = document.createElement("canvas");
  let ctx = canvas.getContext("2d");

  
  
  canvas.width = graphWidth;
  canvas.height = 1;

  
  let imageData = ctx.createImageData(canvas.width, canvas.height);
  let pixelArray = imageData.data;

  let buf = new ArrayBuffer(pixelArray.length);
  let view8bit = new Uint8ClampedArray(buf);
  let view32bit = new Uint32Array(buf);

  
  let [r, g, b] = TIME_INTERVAL_COLOR;
  let alphaComponent = TIME_INTERVAL_OPACITY_MIN;
  let interval = findOptimalTimeInterval(timeScale);

  
  for (let i = 1; i <= TIME_INTERVAL_SCALES; i++) {
    let increment = interval * Math.pow(2, i);
    for (let x = 0; x < canvas.width; x += increment) {
      let position = x | 0;
      view32bit[position] = (alphaComponent << 24) | (b << 16) | (g << 8) | r;
    }
    alphaComponent += TIME_INTERVAL_OPACITY_ADD;
  }

  
  pixelArray.set(view8bit);
  ctx.putImageData(imageData, 0, 0);
  document.mozSetImageElement(id, canvas);
}

exports.drawGraphElementBackground = drawGraphElementBackground;









function findOptimalTimeInterval(timeScale,
                                 minSpacing=TIME_GRADUATION_MIN_SPACING) {
  let timingStep = TIME_INTERVAL_MULTIPLE;
  let numIters = 0;

  if (timeScale > minSpacing) {
    return timeScale;
  }

  while (true) {
    let scaledStep = timeScale * timingStep;
    if (++numIters > OPTIMAL_TIME_INTERVAL_MAX_ITERS) {
      return scaledStep;
    }
    if (scaledStep < minSpacing) {
      timingStep *= 2;
      continue;
    }
    return scaledStep;
  }
}

exports.findOptimalTimeInterval = findOptimalTimeInterval;
