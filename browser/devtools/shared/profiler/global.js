


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");




const STRINGS_URI = "chrome://browser/locale/devtools/profiler.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);





const CATEGORIES = [
  { ordinal: 7, color: "#5e88b0", abbrev: "other", label: L10N.getStr("category.other") },
  { ordinal: 4, color: "#46afe3", abbrev: "css", label: L10N.getStr("category.css") },
  { ordinal: 1, color: "#d96629", abbrev: "js", label: L10N.getStr("category.js") },
  { ordinal: 2, color: "#eb5368", abbrev: "gc", label: L10N.getStr("category.gc") },
  { ordinal: 0, color: "#df80ff", abbrev: "network", label: L10N.getStr("category.network") },
  { ordinal: 5, color: "#70bf53", abbrev: "graphics", label: L10N.getStr("category.graphics") },
  { ordinal: 6, color: "#8fa1b2", abbrev: "storage", label: L10N.getStr("category.storage") },
  { ordinal: 3, color: "#d99b28", abbrev: "events", label: L10N.getStr("category.events") }
];





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
  let mappings = {};
  for (let category of CATEGORIES) {
    let numList = Object.keys(CATEGORY_MAPPINGS)
          .filter(k => CATEGORY_MAPPINGS[k] == category)
          .map(k => +k);
    numList.sort();
    mappings[category.abbrev] = numList;
  }

  return [
    function (name, num) {
      if (!(name in mappings)) {
        throw new Error(`Category abbreviation '${name}' does not exist.`);
      }
      if (arguments.length == 1) {
        if (mappings[name].length != 1) {
          throw new Error(`Expected exactly one category number for '${name}'.`);
        }
        return mappings[name][0];
      }
      if (num > mappings[name].length) {
        throw new Error(`Num '${num}' too high for category '${name}'.`);
      }
      return mappings[name][num - 1];
    },

    function (name) {
      if (!(name in mappings)) {
        throw new Error(`Category abbreviation '${name}' does not exist.`);
      }
      return mappings[name];
    }
  ];
})();




const CATEGORY_OTHER = CATEGORY_MASK('other');



const CATEGORY_JIT = CATEGORY_MASK('js');


exports.L10N = L10N;
exports.CATEGORIES = CATEGORIES;
exports.CATEGORY_MAPPINGS = CATEGORY_MAPPINGS;
exports.CATEGORY_OTHER = CATEGORY_OTHER;
exports.CATEGORY_JIT = CATEGORY_JIT;
exports.CATEGORY_MASK = CATEGORY_MASK;
exports.CATEGORY_MASK_LIST = CATEGORY_MASK_LIST;
