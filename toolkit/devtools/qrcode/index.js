



"use strict";

const { Cu } = require("chrome");
const { Promise: promise } =
  Cu.import("resource://gre/modules/Promise.jsm", {});


Object.defineProperty(this, "Encoder", {
  get: () => require("./encoder/index").Encoder
});
Object.defineProperty(this, "QRRSBlock", {
  get: () => require("./encoder/index").QRRSBlock
});
Object.defineProperty(this, "QRErrorCorrectLevel", {
  get: () => require("./encoder/index").QRErrorCorrectLevel
});
Object.defineProperty(this, "decoder", {
  get: () => require("./decoder/index")
});

















exports.findMinimumVersion = function(message, quality) {
  let msgLength = message.length;
  let qualityLevel = QRErrorCorrectLevel[quality];
  for (let version = 1; version <= 10; version++) {
    let rsBlocks = QRRSBlock.getRSBlocks(version, qualityLevel);
    let maxLength = rsBlocks.reduce((prev, block) => {
      return prev + block.dataCount;
    }, 0);
    
    maxLength -= 2;
    if (msgLength <= maxLength) {
      return version;
    }
  }
  throw new Error("Message too large");
};














exports.encodeToDataURI = function(message, quality, version) {
  quality = quality || "H";
  version = version || exports.findMinimumVersion(message, quality);
  let encoder = new Encoder(version, quality);
  encoder.addData(message);
  encoder.make();
  return encoder.createImgData();
};









exports.decodeFromURI = function(URI) {
  let deferred = promise.defer();
  decoder.decodeFromURI(URI, deferred.resolve, deferred.reject);
  return deferred.promise;
};








exports.decodeFromCanvas = function(canvas) {
  return decoder.decodeFromCanvas(canvas);
};
