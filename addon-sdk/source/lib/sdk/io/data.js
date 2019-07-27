



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci, Cu } = require("chrome");
const base64 = require("../base64");
const IOService = Cc["@mozilla.org/network/io-service;1"].
  getService(Ci.nsIIOService);

const { deprecateFunction } = require('../util/deprecate');
const { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm");
const { Services } = Cu.import("resource://gre/modules/Services.jsm");
const FaviconService = Cc["@mozilla.org/browser/favicon-service;1"].
                          getService(Ci.nsIFaviconService);

const PNG_B64 = "data:image/png;base64,";
const DEF_FAVICON_URI = "chrome://mozapps/skin/places/defaultFavicon.png";
let   DEF_FAVICON = null;








function getFaviconURIForLocation(uri) {
  let pageURI = NetUtil.newURI(uri);
  try {
    return FaviconService.getFaviconDataAsDataURL(
                  FaviconService.getFaviconForPage(pageURI));
  }
  catch(e) {
    if (!DEF_FAVICON) {
      DEF_FAVICON = PNG_B64 +
                    base64.encode(getChromeURIContent(DEF_FAVICON_URI));
    }
    return DEF_FAVICON;
  }
}
exports.getFaviconURIForLocation = getFaviconURIForLocation;






function getChromeURIContent(chromeURI) {
  let channel = IOService.newChannel2(chromeURI,
                                      null,
                                      null,
                                      null,      
                                      Services.scriptSecurityManager.getSystemPrincipal(),
                                      null,      
                                      Ci.nsILoadInfo.SEC_NORMAL,
                                      Ci.nsIContentPolicy.TYPE_OTHER);
  let input = channel.open();
  let stream = Cc["@mozilla.org/binaryinputstream;1"].
                createInstance(Ci.nsIBinaryInputStream);
  stream.setInputStream(input);
  let content = stream.readBytes(input.available());
  stream.close();
  input.close();
  return content;
}
exports.getChromeURIContent = deprecateFunction(getChromeURIContent,
  'getChromeURIContent is deprecated, ' +
  'please use require("sdk/net/url").readURI instead.'
);




exports.base64Encode = deprecateFunction(base64.encode,
  'base64Encode is deprecated, ' +
  'please use require("sdk/base64").encode instead.'
);



exports.base64Decode = deprecateFunction(base64.decode,
  'base64Dencode is deprecated, ' +
  'please use require("sdk/base64").decode instead.'
);
