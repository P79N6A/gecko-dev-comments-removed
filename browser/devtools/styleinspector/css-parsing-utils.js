





"use strict";

const {cssTokenizer} = require("devtools/sourceeditor/css-tokenizer");















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
};
exports.parseDeclarations = parseDeclarations;








function parseSingleValue(value) {
  let declaration = parseDeclarations("a: " + value + ";")[0];
  return {
    value: declaration ? declaration.value : "",
    priority: declaration ? declaration.priority : ""
  };
};
exports.parseSingleValue = parseSingleValue;
