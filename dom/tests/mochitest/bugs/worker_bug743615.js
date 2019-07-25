importScripts('utils_bug743615.js');

self.onmessage = function onMessage(evt) {
  
  var imageData = evt.data.imageData;
  var pattern = evt.data.pattern;
  var statusMessage = checkPattern(imageData, pattern)
                       ? 'PASS' : 'Got corrupt typed array in worker';

  
  if (!(imageData instanceof ImageData))
    statusMessage += ", Bad interface object in worker";

  
  if (imageData.width * imageData.height != imageData.data.length / 4) {
    statusMessage += ", Bad ImageData getters in worker: "
    statusMessage += [imageData.width, imageData.height].join(', ');
  }

  
  var origData = imageData.data;
  var threw = false;
  try {
    imageData.data = [];
    imageData.width = 2;
    imageData.height = 2;
  } catch(e) { threw = true; }
  if (threw || imageData.data !== origData)
    statusMessage = statusMessage + ", Should silently ignore sets";



  
  pattern = makePattern(imageData.data.length, 99, 2);
  setPattern(imageData, pattern);
  self.postMessage({ statusMessage: statusMessage, imageData: imageData,
                     pattern: pattern });
}
