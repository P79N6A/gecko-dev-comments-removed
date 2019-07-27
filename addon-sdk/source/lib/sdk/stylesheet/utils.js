


"use strict";

module.metadata =  {
  "stability": "experimental"
};

const { Ci } = require("chrome");

const SHEET_TYPE = {
  "agent": "AGENT_SHEET",
  "user": "USER_SHEET",
  "author": "AUTHOR_SHEET"
};

function getDOMWindowUtils(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils);
};







function loadSheet(window, url, type) {
  if (!(type && type in SHEET_TYPE))
    type = "author";

  type = SHEET_TYPE[type];

  if (url instanceof Ci.nsIURI)
    url = url.spec;

  let winUtils = getDOMWindowUtils(window);
  try {
    winUtils.loadSheetUsingURIString(url, winUtils[type]);
  }
  catch (e) {};
};
exports.loadSheet = loadSheet;





function removeSheet(window, url, type) {
  if (!(type && type in SHEET_TYPE))
    type = "author";

  type = SHEET_TYPE[type];

  if (url instanceof Ci.nsIURI)
    url = url.spec;

  let winUtils = getDOMWindowUtils(window);

  try {
    winUtils.removeSheetUsingURIString(url, winUtils[type]);
  }
  catch (e) {};
};
exports.removeSheet = removeSheet;





function isTypeValid(type) {
  return type in SHEET_TYPE;
}
exports.isTypeValid = isTypeValid;
