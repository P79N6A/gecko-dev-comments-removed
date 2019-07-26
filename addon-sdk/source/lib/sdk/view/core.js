




"use strict";

module.metadata = {
  "stability": "unstable"
};

var { Ci } = require("chrome");



















let implementations = new WeakMap();

function getNodeView(value) {
  if (value instanceof Ci.nsIDOMNode)
    return value;
  if (implementations.has(value))
    return implementations.get(value)(value);

  return null;
}
getNodeView.implement = function(value, implementation) {
  implementations.set(value, implementation)
}

exports.getNodeView = getNodeView;
