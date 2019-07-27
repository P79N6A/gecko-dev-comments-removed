





"use strict";

const {cssTokenizer} = require("devtools/sourceeditor/css-tokenizer");

const SELECTOR_ATTRIBUTE = exports.SELECTOR_ATTRIBUTE = 1;
const SELECTOR_ELEMENT = exports.SELECTOR_ELEMENT = 2;
const SELECTOR_PSEUDO_CLASS = exports.SELECTOR_PSEUDO_CLASS = 3;















function parseDeclarations(inputString) {
  if (inputString === null || inputString === undefined) {
    throw new Error("empty input string");
  }

  let tokens = cssTokenizer(inputString);

  let declarations = [{name: "", value: "", priority: ""}];

  let current = "", hasBang = false, lastProp;
  for (let token of tokens) {
    lastProp = declarations[declarations.length - 1];

    if (token.tokenType === "symbol" && token.text === ":") {
      if (!lastProp.name) {
        
        lastProp.name = current.trim();
        current = "";
        hasBang = false;
      } else {
        
        
        current += ":";
      }
    } else if (token.tokenType === "symbol" && token.text === ";") {
      lastProp.value = current.trim();
      current = "";
      hasBang = false;
      declarations.push({name: "", value: "", priority: ""});
    } else if (token.tokenType === "ident") {
      if (token.text === "important" && hasBang) {
        lastProp.priority = "important";
        hasBang = false;
      } else {
        if (hasBang) {
          current += "!";
        }
        current += token.text;
      }
    } else if (token.tokenType === "symbol" && token.text === "!") {
      hasBang = true;
    } else if (token.tokenType === "whitespace") {
      current += " ";
    } else if (token.tokenType === "comment") {
      
    } else {
      current += inputString.substring(token.startOffset, token.endOffset);
    }
  }

  
  if (current) {
    if (!lastProp.name) {
      
      lastProp.name = current.trim();
    } else {
      
      lastProp.value += current.trim();
    }
  }

  
  declarations = declarations.filter(prop => prop.name || prop.value);

  return declarations;
}



















function parsePseudoClassesAndAttributes(value) {
  if (!value) {
    throw new Error("empty input string");
  }

  let tokens = cssTokenizer(value);
  let result = [];
  let current = "";
  let functionCount = 0;
  let hasAttribute = false;
  let hasColon = false;

  for (let token of tokens) {
    if (token.tokenType === "ident") {
      current += value.substring(token.startOffset, token.endOffset);

      if (hasColon && !functionCount) {
        if (current) {
          result.push({ value: current, type: SELECTOR_PSEUDO_CLASS });
        }

        current = "";
        hasColon = false;
      }
    } else if (token.tokenType === "symbol" && token.text === ":") {
      if (!hasColon) {
        if (current) {
          result.push({ value: current, type: SELECTOR_ELEMENT });
        }

        current = "";
        hasColon = true;
      }

      current += token.text;
    } else if (token.tokenType === "function") {
      current += value.substring(token.startOffset, token.endOffset);
      functionCount++;
    } else if (token.tokenType === "symbol" && token.text === ")") {
      current += token.text;

      if (hasColon && functionCount == 1) {
        if (current) {
          result.push({ value: current, type: SELECTOR_PSEUDO_CLASS });
        }

        current = "";
        functionCount--;
        hasColon = false;
      } else {
        functionCount--;
      }
    } else if (token.tokenType === "symbol" && token.text === "[") {
      if (!hasAttribute && !functionCount) {
        if (current) {
          result.push({ value: current, type: SELECTOR_ELEMENT });
        }

        current = "";
        hasAttribute = true;
      }

      current += token.text;
    } else if (token.tokenType === "symbol" && token.text === "]") {
      current += token.text;

      if (hasAttribute && !functionCount) {
        if (current) {
          result.push({ value: current, type: SELECTOR_ATTRIBUTE });
        }

        current = "";
        hasAttribute = false;
      }
    } else {
      current += value.substring(token.startOffset, token.endOffset);
    }
  }

  if (current) {
    result.push({ value: current, type: SELECTOR_ELEMENT });
  }

  return result;
}








function parseSingleValue(value) {
  let declaration = parseDeclarations("a: " + value + ";")[0];
  return {
    value: declaration ? declaration.value : "",
    priority: declaration ? declaration.priority : ""
  };
}

exports.parseDeclarations = parseDeclarations;
exports.parsePseudoClassesAndAttributes = parsePseudoClassesAndAttributes;
exports.parseSingleValue = parseSingleValue;
