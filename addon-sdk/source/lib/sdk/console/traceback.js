


"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Ci, components } = require("chrome");
const { parseStack, sourceURI } = require("toolkit/loader");
const { readURISync } = require("../net/url");

function safeGetFileLine(path, line) {
  try {
    var scheme = require("../url").URL(path).scheme;
    
    
    if (!(scheme == "http" || scheme == "https"))
      return readURISync(path).split("\n")[line - 1];
  } catch (e) {}
  return null;
}

function nsIStackFramesToJSON(frame) {
  var stack = [];

  while (frame) {
    if (frame.filename) {
      stack.unshift({
        fileName: sourceURI(frame.filename),
        lineNumber: frame.lineNumber,
        name: frame.name
      });
    }
    frame = frame.caller;
  }

  return stack;
};

var fromException = exports.fromException = function fromException(e) {
  if (e instanceof Ci.nsIException)
    return nsIStackFramesToJSON(e.location);
  if (e.stack && e.stack.length)
    return parseStack(e.stack);
  if (e.fileName && typeof(e.lineNumber == "number"))
    return [{fileName: sourceURI(e.fileName),
             lineNumber: e.lineNumber,
             name: null}];
  return [];
};

var get = exports.get = function get() {
  return nsIStackFramesToJSON(components.stack.caller);
};

var format = exports.format = function format(tbOrException) {
  if (tbOrException === undefined) {
    tbOrException = get();
    tbOrException.pop();
  }

  var tb;
  if (typeof(tbOrException) == "object" &&
      tbOrException.constructor.name == "Array")
    tb = tbOrException;
  else
    tb = fromException(tbOrException);

  var lines = ["Traceback (most recent call last):"];

  tb.forEach(
    function(frame) {
      if (!(frame.fileName || frame.lineNumber || frame.name))
      	return;

      lines.push('  File "' + frame.fileName + '", line ' +
                 frame.lineNumber + ', in ' + frame.name);
      var sourceLine = safeGetFileLine(frame.fileName, frame.lineNumber);
      if (sourceLine)
        lines.push('    ' + sourceLine.trim());
    });

  return lines.join("\n");
};
