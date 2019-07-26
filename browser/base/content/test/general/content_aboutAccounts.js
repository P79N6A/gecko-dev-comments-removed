




"use strict";

addEventListener("DOMContentLoaded", function domContentLoaded(event) {
  removeEventListener("DOMContentLoaded", domContentLoaded);
  sendAsyncMessage("test:document:load");
  let iframe = content.document.getElementById("remote");
  iframe.addEventListener("load", function iframeLoaded(event) {
    if (iframe.contentWindow.location.href == "about:blank" ||
        event.target != iframe) {
      return;
    }
    iframe.removeEventListener("load", iframeLoaded, true);
    sendAsyncMessage("test:iframe:load", {url: iframe.getAttribute("src")});
  }, true);
});
