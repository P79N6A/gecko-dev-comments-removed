


"use strict";

const { Cc, Ci } = require("chrome");

function getZipReader(aFile) {
  return new Promise(resolve => {
    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                        createInstance(Ci.nsIZipReader);
    zipReader.open(aFile);
    resolve(zipReader);
  });
};
exports.getZipReader = getZipReader;
