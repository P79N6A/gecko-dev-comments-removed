


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { deprecateFunction } = require("../util/deprecate");
const { Cc, Ci } = require("chrome");
const XMLHttpRequest = require("../addon/window").window.XMLHttpRequest;

Object.defineProperties(XMLHttpRequest.prototype, {
  mozBackgroundRequest: {
    value: true,
  },
  forceAllowThirdPartyCookie: {
    configurable: true,
    value: deprecateFunction(function() {
      forceAllowThirdPartyCookie(this);

    }, "`xhr.forceAllowThirdPartyCookie()` is deprecated, please use" +
       "`require('sdk/net/xhr').forceAllowThirdPartyCookie(request)` instead")
  }
});
exports.XMLHttpRequest = XMLHttpRequest;

function forceAllowThirdPartyCookie(xhr) {
  if (xhr.channel instanceof Ci.nsIHttpChannelInternal)
    xhr.channel.forceAllowThirdPartyCookie = true;
}
exports.forceAllowThirdPartyCookie = forceAllowThirdPartyCookie;


