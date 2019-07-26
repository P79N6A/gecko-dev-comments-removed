



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cc, Ci, Cr } = require("chrome");
const IOService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
const { isValidURI } = require("../url");
    
function newURI (uri) {
  if (!isValidURI(uri))
    throw new Error("malformed URI: " + uri);
  return IOService.newURI(uri, null, null);
}
exports.newURI = newURI;
