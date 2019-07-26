



"use strict";

let { Encoder, QRRSBlock, QRErrorCorrectLevel } = require("./encoder/index");

















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
