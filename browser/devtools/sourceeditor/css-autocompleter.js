



const { Cc, Ci, Cu } = require('chrome');
const cssTokenizer  = require("devtools/sourceeditor/css-tokenizer");
const promise = Cu.import("resource://gre/modules/Promise.jsm");














































const CSS_STATES = {
  "null": "null",
  property: "property",    
  value: "value",          
  selector: "selector",    
  media: "media",          
  keyframes: "keyframes",  
  frame: "frame",          
};

const SELECTOR_STATES = {
  "null": "null",
  id: "id",                
  class: "class",          
  tag: "tag",              
  pseudo: "pseudo",        
  attribute: "attribute",  
  value: "value",          
};

const { properties, propertyNames } = getCSSKeywords();









function CSSCompleter(options = {}) {
  this.walker = options.walker;
  this.maxEntries = options.maxEntries || 15;
}

CSSCompleter.prototype = {

  










  complete: function(source, caret) {
    
    if (!this.resolveState(source, caret)) {
      
      return Promise.resolve([]);
    }

    
    switch(this.state) {
      case CSS_STATES.property:
        return this.completeProperties(this.completing);

      case CSS_STATES.value:
        return this.completeValues(this.propertyName, this.completing);

      case CSS_STATES.selector:
        return this.suggestSelectors();

      case CSS_STATES.media:
      case CSS_STATES.keyframes:
        if ("media".startsWith(this.completing)) {
          return Promise.resolve([{
            label: "media",
            preLabel: this.completing
          }]);
        } else if ("keyframes".startsWith(this.completing)) {
          return Promise.resolve([{
            label: "keyrames",
            preLabel: this.completing
          }]);
        }
    }
    return Promise.resolve([]);
  },

  












  resolveState: function(source, {line, ch}) {
    
    let peek = arr => arr[arr.length - 1];
    let tokens = cssTokenizer(source, {loc:true});
    let tokIndex = tokens.length - 1;
    if (tokens[tokIndex].loc.end.line < line ||
       (tokens[tokIndex].loc.end.line === line &&
        tokens[tokIndex].loc.end.column < ch)) {
      
      
      
      return null;
    }
    
    tokIndex--;

    
    let _state = CSS_STATES.null;
    let cursor = 0;
    
    let scopeStack = [];
    let token = null;
    let propertyName = null;
    let selector = "";
    let selectorBeforeNot = "";
    let selectorState = SELECTOR_STATES.null;
    while (cursor <= tokIndex && (token = tokens[cursor++])) {
      switch (_state) {
        case CSS_STATES.property:
          
          
          switch(token.tokenType) {
            case ":":
              scopeStack.push(":");
              if (tokens[cursor - 2].tokenType != "WHITESPACE")
                propertyName = tokens[cursor - 2].value;
              else
                propertyName = tokens[cursor - 3].value;
              _state = CSS_STATES.value;
              break;

            case "}":
              if (/[{f]/.test(peek(scopeStack))) {
                let popped = scopeStack.pop();
                if (popped == "f") {
                  _state = CSS_STATES.frame;
                } else {
                  selector = "";
                  _state = CSS_STATES.null;
                }
              }
              break;
          }
          break;

        case CSS_STATES.value:
          
          
          switch(token.tokenType) {
            case ";":
              if (/[:]/.test(peek(scopeStack))) {
                scopeStack.pop();
                _state = CSS_STATES.property;
              }
              break;

            case "}":
              if (peek(scopeStack) == ":")
                scopeStack.pop();

              if (/[{f]/.test(peek(scopeStack))) {
                let popped = scopeStack.pop();
                if (popped == "f") {
                  _state = CSS_STATES.frame;
                } else {
                  selector = "";
                  _state = CSS_STATES.null;
                }
              }
              break;
          }
          break;

        case CSS_STATES.selector:
          
          
          if (token.tokenType == "{") {
            scopeStack.push("{");
            _state = CSS_STATES.property;
            break;
          }
          switch(selectorState) {
            case SELECTOR_STATES.id:
            case SELECTOR_STATES.class:
            case SELECTOR_STATES.tag:
              switch(token.tokenType) {
                case "HASH":
                  selectorState = SELECTOR_STATES.id;
                  selector += "#" + token.value;
                  break;

                case "DELIM":
                  if (token.value == ".") {
                    selectorState = SELECTOR_STATES.class;
                    selector += ".";
                    if (cursor <= tokIndex &&
                        tokens[cursor].tokenType == "IDENT") {
                      token = tokens[cursor++];
                      selector += token.value;
                    }
                  } else if (token.value == "#") {
                    selectorState = SELECTOR_STATES.id;
                    selector += "#";
                  } else if (/[>~+]/.test(token.value)) {
                    selectorState = SELECTOR_STATES.null;
                    selector += token.value;
                  } else if (token.value == ",") {
                    selectorState = SELECTOR_STATES.null;
                    selector = "";
                  }
                  break;

                case ":":
                  selectorState = SELECTOR_STATES.pseudo;
                  selector += ":";
                  if (cursor > tokIndex)
                    break;

                  token = tokens[cursor++];
                  switch(token.tokenType) {
                    case "FUNCTION":
                      selectorState = SELECTOR_STATES.null;
                      selectorBeforeNot = selector;
                      selector = "";
                      scopeStack.push("(");
                      break;

                    case "IDENT":
                      selector += token.value;
                      break;
                  }
                  break;

                case "[":
                  selectorState = SELECTOR_STATES.attribute;
                  scopeStack.push("[");
                  selector += "[";
                  break;

                case ")":
                  if (peek(scopeStack) == "(") {
                    scopeStack.pop();
                    selector = selectorBeforeNot + "not(" + selector + ")";
                    selectorState = SELECTOR_STATES.null;
                  }
                  break;

                case "WHITESPACE":
                  selectorState = SELECTOR_STATES.null;
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.null:
              
              
              switch(token.tokenType) {
                case "HASH":
                  selectorState = SELECTOR_STATES.id;
                  selector += "#" + token.value;
                  break;

                case "IDENT":
                  selectorState = SELECTOR_STATES.tag;
                  selector += token.value;
                  break;

                case "DELIM":
                  if (token.value == ".") {
                    selectorState = SELECTOR_STATES.class;
                    selector += ".";
                    if (cursor <= tokIndex &&
                        tokens[cursor].tokenType == "IDENT") {
                      token = tokens[cursor++];
                      selector += token.value;
                    }
                  } else if (token.value == "#") {
                    selectorState = SELECTOR_STATES.id;
                    selector += "#";
                  } else if (token.value == "*") {
                    selectorState = SELECTOR_STATES.tag;
                    selector += "*";
                  } else if (/[>~+]/.test(token.value)) {
                    selector += token.value;
                  } else if (token.value == ",") {
                    selectorState = SELECTOR_STATES.null;
                    selector = "";
                  }
                  break;

                case ":":
                  selectorState = SELECTOR_STATES.pseudo;
                  selector += ":";
                  if (cursor > tokIndex)
                    break;

                  token = tokens[cursor++];
                  switch(token.tokenType) {
                    case "FUNCTION":
                      selectorState = SELECTOR_STATES.null;
                      selectorBeforeNot = selector;
                      selector = "";
                      scopeStack.push("(");
                      break;

                    case "IDENT":
                      selector += token.value;
                      break;
                  }
                  break;

                case "[":
                  selectorState = SELECTOR_STATES.attribute;
                  scopeStack.push("[");
                  selector += "[";
                  break;

                case ")":
                  if (peek(scopeStack) == "(") {
                    scopeStack.pop();
                    selector = selectorBeforeNot + "not(" + selector + ")";
                    selectorState = SELECTOR_STATES.null;
                  }
                  break;

                case "WHITESPACE":
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.pseudo:
              switch(token.tokenType) {
                case "DELIM":
                  if (/[>~+]/.test(token.value)) {
                    selectorState = SELECTOR_STATES.null;
                    selector += token.value;
                  } else if (token.value == ",") {
                    selectorState = SELECTOR_STATES.null;
                    selector = "";
                  }
                  break;

                case ":":
                  selectorState = SELECTOR_STATES.pseudo;
                  selector += ":";
                  if (cursor > tokIndex)
                    break;

                  token = tokens[cursor++];
                  switch(token.tokenType) {
                    case "FUNCTION":
                      selectorState = SELECTOR_STATES.null;
                      selectorBeforeNot = selector;
                      selector = "";
                      scopeStack.push("(");
                      break;

                    case "IDENT":
                      selector += token.value;
                      break;
                  }
                  break;

                case "[":
                  selectorState = SELECTOR_STATES.attribute;
                  scopeStack.push("[");
                  selector += "[";
                  break;

                case "WHITESPACE":
                  selectorState = SELECTOR_STATES.null;
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.attribute:
              switch(token.tokenType) {
                case "DELIM":
                  if (/[~|^$*]/.test(token.value)) {
                    selector += token.value;
                    token = tokens[cursor++];
                  }
                  if(token.value == "=") {
                    selectorState = SELECTOR_STATES.value;
                    selector += token.value;
                  }
                  break;

                case "IDENT":
                case "STRING":
                  selector += token.value;
                  break;

                case "]":
                  if (peek(scopeStack) == "[")
                    scopeStack.pop();

                  selectorState = SELECTOR_STATES.null;
                  selector += "]";
                  break;

                case "WHITESPACE":
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.value:
              switch(token.tokenType) {
                case "STRING":
                case "IDENT":
                  selector += token.value;
                  break;

                case "]":
                  if (peek(scopeStack) == "[")
                    scopeStack.pop();

                  selectorState = SELECTOR_STATES.null;
                  selector += "]";
                  break;

                case "WHITESPACE":
                  selector && (selector += " ");
                  break;
              }
              break;
          }
          break;

        case CSS_STATES.null:
          
          
          switch(token.tokenType) {
            case "HASH":
              selectorState = SELECTOR_STATES.id;
              selector = "#" + token.value;
              _state = CSS_STATES.selector;
              break;

            case "IDENT":
              selectorState = SELECTOR_STATES.tag;
              selector = token.value;
              _state = CSS_STATES.selector;
              break;

            case "DELIM":
              if (token.value == ".") {
                selectorState = SELECTOR_STATES.class;
                selector = ".";
                _state = CSS_STATES.selector;
                if (cursor <= tokIndex &&
                    tokens[cursor].tokenType == "IDENT") {
                  token = tokens[cursor++];
                  selector += token.value;
                }
              } else if (token.value == "#") {
                selectorState = SELECTOR_STATES.id;
                selector = "#";
                _state = CSS_STATES.selector;
              } else if (token.value == "*") {
                selectorState = SELECTOR_STATES.tag;
                selector = "*";
                _state = CSS_STATES.selector;
              }
              break;

            case ":":
              _state = CSS_STATES.selector;
              selectorState = SELECTOR_STATES.pseudo;
              selector += ":";
              if (cursor > tokIndex)
                break;

              token = tokens[cursor++];
              switch(token.tokenType) {
                case "FUNCTION":
                  selectorState = SELECTOR_STATES.null;
                  selectorBeforeNot = selector;
                  selector = "";
                  scopeStack.push("(");
                  break;

                case "IDENT":
                  selector += token.value;
                  break;
              }
              break;

            case "[":
              _state = CSS_STATES.selector;
              selectorState = SELECTOR_STATES.attribute;
              scopeStack.push("[");
              selector += "[";
              break;

            case "AT-KEYWORD":
              _state = token.value.startsWith("m") ? CSS_STATES.media
                                                   : CSS_STATES.keyframes;
              break;

            case "}":
              if (peek(scopeStack) == "@m")
                scopeStack.pop();

              break;
          }
          break;

        case CSS_STATES.media:
          
          
          if (token.tokenType == "{") {
            scopeStack.push("@m");
            _state = CSS_STATES.null;
          }
          break;

        case CSS_STATES.keyframes:
          
          
          if (token.tokenType == "{") {
            scopeStack.push("@k");
            _state = CSS_STATES.frame;
          }
          break;

        case CSS_STATES.frame:
          
          
          if (token.tokenType == "{") {
            scopeStack.push("f");
            _state = CSS_STATES.property;
          } else if (token.tokenType == "}") {
            if (peek(scopeStack) == "@k")
              scopeStack.pop();

            _state = CSS_STATES.null;
          }
          break;
      }
    }
    this.state = _state;
    if (!token)
      return _state;

    if (token && token.tokenType != "WHITESPACE") {
      this.completing = ((token.value || token.repr || token.tokenType) + "")
                          .slice(0, ch - token.loc.start.column)
                          .replace(/^[.#]$/, "");
    } else {
      this.completing = "";
    }
    
    if (tokens[cursor - 2] && tokens[cursor - 2].value == "!" &&
        this.completing == "important".slice(0, this.completing.length)) {
      this.completing = "!" + this.completing;
    }
    this.propertyName = _state == CSS_STATES.value ? propertyName : null;
    selector = selector.slice(0, selector.length + token.loc.end.column - ch);
    this.selector = _state == CSS_STATES.selector ? selector : null;
    this.selectorState = _state == CSS_STATES.selector ? selectorState : null;
    return _state;
  },

  



  suggestSelectors: function () {
    let walker = this.walker;
    if (!walker)
      return Promise.resolve([]);

    let query = this.selector;
    
    
    switch(this.selectorState) {
      case SELECTOR_STATES.null:
        query += "*";
        break;

      case SELECTOR_STATES.tag:
        query = query.slice(0, query.length - this.completing.length);
        break;

      case SELECTOR_STATES.id:
      case SELECTOR_STATES.class:
      case SELECTOR_STATES.pseudo:
        if (/^[.:#]$/.test(this.completing)) {
          query = query.slice(0, query.length - this.completing.length);
          this.completing = "";
        } else {
          query = query.slice(0, query.length - this.completing.length - 1);
        }
        break;
    }

    if (/[\s+>~]$/.test(query) &&
        this.selectorState != SELECTOR_STATES.attribute &&
        this.selectorState != SELECTOR_STATES.value) {
      query += "*";
    }

    
    this._currentQuery = query;
    return walker.getSuggestionsForQuery(query, this.completing, this.selectorState)
                 .then(result => this.prepareSelectorResults(result));
  },

 


  prepareSelectorResults: function(result) {
    if (this._currentQuery != result.query)
      return [];

    result = result.suggestions;
    let query = this.selector;
    let completion = [];
    for (let value of result) {
      switch(this.selectorState) {
        case SELECTOR_STATES.id:
        case SELECTOR_STATES.class:
        case SELECTOR_STATES.pseudo:
          if (/^[.:#]$/.test(this.completing)) {
            value[0] = query.slice(0, query.length - this.completing.length) +
                       value[0];
          } else {
            value[0] = query.slice(0, query.length - this.completing.length - 1) +
                       value[0];
          }
          break;

        case SELECTOR_STATES.tag:
          value[0] = query.slice(0, query.length - this.completing.length) +
                     value[0];
          break;

        case SELECTOR_STATES.null:
          value[0] = query + value[0];
          break;

        default:
         value[0] = query.slice(0, query.length - this.completing.length) +
                    value[0];
      }
      completion.push({
        label: value[0],
        preLabel: query,
        score: value[1]
      });
      if (completion.length > this.maxEntries - 1)
        break;
    }
    return completion;
  },

  




  completeProperties: function(startProp) {
    let finalList = [];
    let length = propertyNames.length;
    let i = 0, count = 0;
    for (; i < length && count < this.maxEntries; i++) {
      if (propertyNames[i].startsWith(startProp)) {
        count++;
        finalList.push({
          preLabel: startProp,
          label: propertyNames[i]
        });
      } else if (propertyNames[i] > startProp) {
        
        break;
      }
    }
    return Promise.resolve(finalList);
  },

  






  completeValues: function(propName, startValue) {
    let finalList = [];
    let list = ["!important;", ...(properties[propName] || [])];
    let length = list.length;
    let i = 0, count = 0;
    for (; i < length && count < this.maxEntries; i++) {
      if (list[i].startsWith(startValue)) {
        count++;
        finalList.push({
          preLabel: startValue,
          label: list[i]
        });
      } else if (list[i] > startValue) {
        
        break;
      }
    }
    return Promise.resolve(finalList);
  },
}












function getCSSKeywords() {
  let domUtils = Cc["@mozilla.org/inspector/dom-utils;1"]
                   .getService(Ci.inIDOMUtils);
  let props = {};
  let propNames = domUtils.getCSSPropertyNames(domUtils.INCLUDE_ALIASES);
  propNames.forEach(prop => {
    props[prop] = domUtils.getCSSValuesForProperty(prop).sort();
  });
  return {
    properties: props,
    propertyNames: propNames.sort()
  };
}

module.exports = CSSCompleter;
