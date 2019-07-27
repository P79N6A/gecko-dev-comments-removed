


"use strict";

const { ViewHelpers } = require("resource:///modules/devtools/ViewHelpers.jsm");





const L10N = new ViewHelpers.MultiL10N([
  "chrome://browser/locale/devtools/timeline.properties",
  "chrome://browser/locale/devtools/profiler.properties"
]);





const PREFS = new ViewHelpers.Prefs("devtools.performance", {
  "show-platform-data": ["Bool", "ui.show-platform-data"],
  "hidden-markers": ["Json", "timeline.hidden-markers"],
  "memory-sample-probability": ["Float", "memory.sample-probability"],
  "memory-max-log-length": ["Int", "memory.max-log-length"],
  "profiler-buffer-size": ["Int", "profiler.buffer-size"],
  "profiler-sample-frequency": ["Int", "profiler.sample-frequency-khz"],
}, {
  monitorChanges: true
});





const CATEGORIES = [{
  color: "#5e88b0",
  abbrev: "other",
  label: L10N.getStr("category.other")
}, {
  color: "#46afe3",
  abbrev: "css",
  label: L10N.getStr("category.css")
}, {
  color: "#d96629",
  abbrev: "js",
  label: L10N.getStr("category.js")
}, {
  color: "#eb5368",
  abbrev: "gc",
  label: L10N.getStr("category.gc")
}, {
  color: "#df80ff",
  abbrev: "network",
  label: L10N.getStr("category.network")
}, {
  color: "#70bf53",
  abbrev: "graphics",
  label: L10N.getStr("category.graphics")
}, {
  color: "#8fa1b2",
  abbrev: "storage",
  label: L10N.getStr("category.storage")
}, {
  color: "#d99b28",
  abbrev: "events",
  label: L10N.getStr("category.events")
}];





const CATEGORY_MAPPINGS = {
  "16": CATEGORIES[0],    
  "32": CATEGORIES[1],    
  "64": CATEGORIES[2],    
  "128": CATEGORIES[3],   
  "256": CATEGORIES[3],   
  "512": CATEGORIES[4],   
  "1024": CATEGORIES[5],  
  "2048": CATEGORIES[6],  
  "4096": CATEGORIES[7],  
};










const [CATEGORY_MASK, CATEGORY_MASK_LIST] = (function () {
  let bitmasksForCategory = {};
  let all = Object.keys(CATEGORY_MAPPINGS);

  for (let category of CATEGORIES) {
    bitmasksForCategory[category.abbrev] = all
      .filter(mask => CATEGORY_MAPPINGS[mask] == category)
      .map(mask => +mask)
      .sort();
  }

  return [
    function (name, index) {
      if (!(name in bitmasksForCategory)) {
        throw new Error(`Category abbreviation '${name}' does not exist.`);
      }
      if (arguments.length == 1) {
        if (bitmasksForCategory[name].length != 1) {
          throw new Error(`Expected exactly one category number for '${name}'.`);
        } else {
          return bitmasksForCategory[name][0];
        }
      } else {
        if (index > bitmasksForCategory[name].length) {
          throw new Error(`Index '${index}' too high for category '${name}'.`);
        } else {
          return bitmasksForCategory[name][index - 1];
        }
      }
    },

    function (name) {
      if (!(name in bitmasksForCategory)) {
        throw new Error(`Category abbreviation '${name}' does not exist.`);
      }
      return bitmasksForCategory[name];
    }
  ];
})();




const CATEGORY_OTHER = CATEGORY_MASK('other');



const CATEGORY_JIT = CATEGORY_MASK('js');


exports.L10N = L10N;
exports.PREFS = PREFS;
exports.CATEGORIES = CATEGORIES;
exports.CATEGORY_MAPPINGS = CATEGORY_MAPPINGS;
exports.CATEGORY_MASK = CATEGORY_MASK;
exports.CATEGORY_MASK_LIST = CATEGORY_MASK_LIST;
exports.CATEGORY_OTHER = CATEGORY_OTHER;
exports.CATEGORY_JIT = CATEGORY_JIT;
