



"use strict";

module.metadata = {
  "stability": "unstable",
  "engines": {
    "Firefox": "*"
  }
};

const { Cc, Ci, Cu } = require("chrome");
const { defer, reject } = require("../core/promise");
const FaviconService = Cc["@mozilla.org/browser/favicon-service;1"].
                          getService(Ci.nsIFaviconService);
const AsyncFavicons = FaviconService.QueryInterface(Ci.mozIAsyncFavicons);
const { isValidURI } = require("../url");
const { newURI, getURL } = require("../url/utils");









function getFavicon (object, callback) {
  let url = getURL(object);
  let deferred = defer();

  if (url && isValidURI(url)) {
    AsyncFavicons.getFaviconURLForPage(newURI(url), function (aURI) {
      if (aURI && aURI.spec)
        deferred.resolve(aURI.spec.toString());
      else
        deferred.reject(null);
    });
  } else {
    deferred.reject(null);
  }

  if (callback) deferred.promise.then(callback, callback);
  return deferred.promise;
}
exports.getFavicon = getFavicon;
