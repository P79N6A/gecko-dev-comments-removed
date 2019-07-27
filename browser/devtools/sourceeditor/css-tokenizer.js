





"use strict";

const {Cc, Ci} = require("chrome");
loader.lazyGetter(this, "DOMUtils", () => {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});









function* cssTokenizer(string) {
  let lexer = DOMUtils.getCSSLexer(string);
  while (true) {
    let token = lexer.nextToken();
    if (!token) {
      break;
    }
    
    if (token.tokenType !== "comment") {
      yield token;
    }
  }
}

exports.cssTokenizer = cssTokenizer;




















function cssTokenizerWithLineColumn(string) {
  let lexer = DOMUtils.getCSSLexer(string);
  let result = [];
  let prevToken = undefined;
  while (true) {
    let token = lexer.nextToken();
    let lineNumber = lexer.lineNumber;
    let columnNumber = lexer.columnNumber;

    if (prevToken) {
      prevToken.loc.end = {
        line: lineNumber,
        column: columnNumber
      };
    }

    if (!token) {
      break;
    }

    if (token.tokenType === "comment") {
      
      prevToken = undefined;
    } else {
      let startLoc = {
        line: lineNumber,
        column: columnNumber
      };
      token.loc = {start: startLoc};

      result.push(token);
      prevToken = token;
    }
  }

  return result;
}

exports.cssTokenizerWithLineColumn = cssTokenizerWithLineColumn;
