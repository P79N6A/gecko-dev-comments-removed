


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
  "8": CATEGORIES[0],    
  "16": CATEGORIES[1],   
  "32": CATEGORIES[2],   
  "64": CATEGORIES[3],   
  "128": CATEGORIES[3],  
  "256": CATEGORIES[4],  
  "512": CATEGORIES[5],  
  "1024": CATEGORIES[6], 
  "2048": CATEGORIES[7], 
};




const CATEGORY_OTHER = 8;



const CATEGORY_JIT = 32;


exports.L10N = L10N;
exports.CATEGORIES = CATEGORIES;
exports.CATEGORY_MAPPINGS = CATEGORY_MAPPINGS;
exports.CATEGORY_OTHER = CATEGORY_OTHER;
exports.CATEGORY_JIT = CATEGORY_JIT;
