



"use strict";

this.EXPORTED_SYMBOLS = ["ScrollPosition"];

const Ci = Components.interfaces;







this.ScrollPosition = Object.freeze({
  








  collect: function (frame) {
    let ifreq = frame.QueryInterface(Ci.nsIInterfaceRequestor);
    let utils = ifreq.getInterface(Ci.nsIDOMWindowUtils);
    let scrollX = {}, scrollY = {};
    utils.getScrollXY(false , scrollX, scrollY);

    if (scrollX.value || scrollY.value) {
      return {scroll: scrollX.value + "," + scrollY.value};
    }

    return null;
  },

  





  restore: function (frame, value) {
    let match;

    if (value && (match = /(\d+),(\d+)/.exec(value))) {
      frame.scrollTo(match[1], match[2]);
    }
  },

  



















  restoreTree: function (root, data) {
    if (data.hasOwnProperty("scroll")) {
      this.restore(root, data.scroll);
    }

    if (!data.hasOwnProperty("children")) {
      return;
    }

    let frames = root.frames;
    data.children.forEach((child, index) => {
      if (child && index < frames.length) {
        this.restoreTree(frames[index], child);
      }
    });
  }
});
