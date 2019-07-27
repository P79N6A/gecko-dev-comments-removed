





"use strict";

const cssTokenizer  = require("devtools/sourceeditor/css-tokenizer");




function quoteString(string) {
  let hasDoubleQuotes = string.includes('"');
  let hasSingleQuotes = string.includes("'");

  let quote = '"';
  if (hasDoubleQuotes && !hasSingleQuotes) {
    quote = "'";
  }

  
  
  
  return quote +
    string.replace(/[\\"]/g, match => {
      switch (match) {
      case '\\':
        return '\\\\';
      case '"':
        if (quote == '"')
          return '\\"';
        return match;
      }
    }) +
    quote;
}















function parseDeclarations(inputString) {
  let tokens = cssTokenizer(inputString);

  let declarations = [{name: "", value: "", priority: ""}];

  let current = "", hasBang = false, lastProp;
  for (let token of tokens) {
    lastProp = declarations[declarations.length - 1];

    if (token.tokenType === ":") {
      if (!lastProp.name) {
        
        lastProp.name = current.trim();
        current = "";
        hasBang = false;
      } else {
        
        
        current += ":";
      }
    } else if (token.tokenType === ";") {
      lastProp.value = current.trim();
      current = "";
      hasBang = false;
      declarations.push({name: "", value: "", priority: ""});
    } else {
      switch(token.tokenType) {
        case "IDENT":
          if (token.value === "important" && hasBang) {
            lastProp.priority = "important";
            hasBang = false;
          } else {
            if (hasBang) {
              current += "!";
            }
            current += token.value;
          }
          break;
        case "WHITESPACE":
          current += " ";
          break;
        case "DIMENSION":
          current += token.repr;
          break;
        case "HASH":
          current += "#" + token.value;
          break;
        case "URL":
          current += "url(" + quoteString(token.value) + ")";
          break;
        case "FUNCTION":
          current += token.value + "(";
          break;
        case "(":
        case ")":
          current += token.tokenType;
          break;
        case "EOF":
          break;
        case "DELIM":
          if (token.value === "!") {
            hasBang = true;
          } else {
            current += token.value;
          }
          break;
        case "STRING":
          current += quoteString(token.value);
          break;
        case "{":
        case "}":
          current += token.tokenType;
          break;
        default:
          current += token.value;
          break;
      }
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
