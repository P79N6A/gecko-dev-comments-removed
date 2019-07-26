



"use strict";

const hiddenFrames = require("sdk/frame/hidden-frame");
const { HiddenFrame } = hiddenFrames;

exports["test Frame"] = function(assert, done) {
  let url = "data:text/html;charset=utf-8,<!DOCTYPE%20html>";

  let hiddenFrame = hiddenFrames.add(HiddenFrame({
    onReady: function () {
      assert.equal(this.element.contentWindow.location, "about:blank",
                       "HiddenFrame loads about:blank by default.");

      function onDOMReady() {
        hiddenFrame.element.removeEventListener("DOMContentLoaded", onDOMReady,
                                                false);
        assert.equal(hiddenFrame.element.contentWindow.location, url,
                         "HiddenFrame loads the specified content.");
        done();
      }

      this.element.addEventListener("DOMContentLoaded", onDOMReady, false);
      this.element.setAttribute("src", url);
    }
  }));
};

require("test").run(exports);
