




"use strict";

module.metadata = {
  "stability": "unstable"
};

var { Ci } = require("chrome");
var method = require("method/core");





let getNodeView = method("getNodeView");
getNodeView.define(function(value) {
  if (value instanceof Ci.nsIDOMNode)
    return value;
  return null;
});

exports.getNodeView = getNodeView;

let getActiveView = method("getActiveView");
exports.getActiveView = getActiveView;
