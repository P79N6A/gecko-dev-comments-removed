



"use strict";

var customHeader = "custom-key";
var customHeaderValue = "custom-key-value";

self.onmessage = function(event) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", event.data, false);
  xhr.setRequestHeader(customHeader, customHeaderValue);
  xhr.send();
  postMessage({ response: xhr.responseText, header: customHeader });
}
