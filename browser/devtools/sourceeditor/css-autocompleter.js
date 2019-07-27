



const { Cc, Ci, Cu } = require('chrome');
const {cssTokenizer, cssTokenizerWithLineColumn}  =
      require("devtools/sourceeditor/css-tokenizer");
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

  
  
  this.nullStates = [];
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
            preLabel: this.completing,
            text: "media"
          }]);
        } else if ("keyframes".startsWith(this.completing)) {
          return Promise.resolve([{
            label: "keyframes",
            preLabel: this.completing,
            text: "keyframes"
          }]);
        }
    }
    return Promise.resolve([]);
  },

  












  resolveState: function(source, {line, ch}) {
    
    let peek = arr => arr[arr.length - 1];
    
    let _state = CSS_STATES.null;
    let selector = "";
    let selectorState = SELECTOR_STATES.null;
    let propertyName = null;
    let scopeStack = [];
    let selectors = [];

    
    let matchedStateIndex = this.findNearestNullState(line);
    if (matchedStateIndex > -1) {
      let state = this.nullStates[matchedStateIndex];
      line -= state[0];
      if (line == 0)
        ch -= state[1];
      source = source.split("\n").slice(state[0]);
      source[0] = source[0].slice(state[1]);
      source = source.join("\n");
      scopeStack = [...state[2]];
      this.nullStates.length = matchedStateIndex + 1;
    }
    else {
      this.nullStates = [];
    }
    let tokens = cssTokenizerWithLineColumn(source);
    let tokIndex = tokens.length - 1;
    if (tokIndex >=0 &&
        (tokens[tokIndex].loc.end.line < line ||
         (tokens[tokIndex].loc.end.line === line &&
          tokens[tokIndex].loc.end.column < ch))) {
      
      
      
      return null;
    }

    let cursor = 0;
    
    let token = null;
    let selectorBeforeNot = null;
    while (cursor <= tokIndex && (token = tokens[cursor++])) {
      switch (_state) {
        case CSS_STATES.property:
          
          
          if (token.tokenType === "symbol") {
            switch(token.text) {
            case ":":
              scopeStack.push(":");
              if (tokens[cursor - 2].tokenType != "whitespace")
                propertyName = tokens[cursor - 2].text;
              else
                propertyName = tokens[cursor - 3].text;
              _state = CSS_STATES.value;
              break;

            case "}":
              if (/[{f]/.test(peek(scopeStack))) {
                let popped = scopeStack.pop();
                if (popped == "f") {
                  _state = CSS_STATES.frame;
                } else {
                  selector = "";
                  selectors = [];
                  _state = CSS_STATES.null;
                }
              }
              break;
            }
          }
          break;

        case CSS_STATES.value:
          
          
          if (token.tokenType === "symbol") {
            switch(token.text) {
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
                  selectors = [];
                  _state = CSS_STATES.null;
                }
              }
              break;
            }
          }
          break;

        case CSS_STATES.selector:
          
          
          if (token.tokenType === "symbol" && token.text == "{") {
            scopeStack.push("{");
            _state = CSS_STATES.property;
            selectors.push(selector);
            selector = "";
            break;
          }
          switch(selectorState) {
            case SELECTOR_STATES.id:
            case SELECTOR_STATES.class:
            case SELECTOR_STATES.tag:
              switch(token.tokenType) {
                case "hash":
                case "id":
                  selectorState = SELECTOR_STATES.id;
                  selector += "#" + token.text;
                  break;

                case "symbol":
                  if (token.text == ".") {
                    selectorState = SELECTOR_STATES.class;
                    selector += ".";
                    if (cursor <= tokIndex &&
                        tokens[cursor].tokenType == "ident") {
                      token = tokens[cursor++];
                      selector += token.text;
                    }
                  } else if (token.text == "#") {
                    selectorState = SELECTOR_STATES.id;
                    selector += "#";
                  } else if (/[>~+]/.test(token.text)) {
                    selectorState = SELECTOR_STATES.null;
                    selector += token.text;
                  } else if (token.text == ",") {
                    selectorState = SELECTOR_STATES.null;
                    selectors.push(selector);
                    selector = "";
                  } else if (token.text == ":") {
                    selectorState = SELECTOR_STATES.pseudo;
                    selector += ":";
                    if (cursor > tokIndex)
                      break;

                    token = tokens[cursor++];
                    switch(token.tokenType) {
                      case "function":
                        if (token.text == "not") {
                          selectorBeforeNot = selector;
                          selector = "";
                          scopeStack.push("(");
                        } else {
                          selector += token.text + "(";
                        }
                        selectorState = SELECTOR_STATES.null;
                        break;

                      case "ident":
                        selector += token.text;
                        break;
                    }
                  } else if (token.text == "[") {
                    selectorState = SELECTOR_STATES.attribute;
                    scopeStack.push("[");
                    selector += "[";
                  } else if (token.text == ")") {
                    if (peek(scopeStack) == "(") {
                      scopeStack.pop();
                      selector = selectorBeforeNot + "not(" + selector + ")";
                      selectorBeforeNot = null;
                    } else {
                      selector += ")";
                    }
                    selectorState = SELECTOR_STATES.null;
                  }
                  break;

                case "whitespace":
                  selectorState = SELECTOR_STATES.null;
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.null:
              
              
              switch(token.tokenType) {
                case "hash":
                case "id":
                  selectorState = SELECTOR_STATES.id;
                  selector += "#" + token.text;
                  break;

                case "ident":
                  selectorState = SELECTOR_STATES.tag;
                  selector += token.text;
                  break;

                case "symbol":
                  if (token.text == ".") {
                    selectorState = SELECTOR_STATES.class;
                    selector += ".";
                    if (cursor <= tokIndex &&
                        tokens[cursor].tokenType == "ident") {
                      token = tokens[cursor++];
                      selector += token.text;
                    }
                  } else if (token.text == "#") {
                    selectorState = SELECTOR_STATES.id;
                    selector += "#";
                  } else if (token.text == "*") {
                    selectorState = SELECTOR_STATES.tag;
                    selector += "*";
                  } else if (/[>~+]/.test(token.text)) {
                    selector += token.text;
                  } else if (token.text == ",") {
                    selectorState = SELECTOR_STATES.null;
                    selectors.push(selector);
                    selector = "";
                  } else if (token.text == ":") {
                    selectorState = SELECTOR_STATES.pseudo;
                    selector += ":";
                    if (cursor > tokIndex)
                      break;

                    token = tokens[cursor++];
                    switch(token.tokenType) {
                      case "function":
                        if (token.text == "not") {
                          selectorBeforeNot = selector;
                          selector = "";
                          scopeStack.push("(");
                        } else {
                          selector += token.text + "(";
                        }
                        selectorState = SELECTOR_STATES.null;
                        break;

                      case "ident":
                        selector += token.text;
                        break;
                    }
                  } else if (token.text == "[") {
                    selectorState = SELECTOR_STATES.attribute;
                    scopeStack.push("[");
                    selector += "[";
                  } else if (token.text == ")") {
                    if (peek(scopeStack) == "(") {
                      scopeStack.pop();
                      selector = selectorBeforeNot + "not(" + selector + ")";
                      selectorBeforeNot = null;
                    } else {
                      selector += ")";
                    }
                    selectorState = SELECTOR_STATES.null;
                  }
                  break;

                case "whitespace":
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.pseudo:
              switch(token.tokenType) {
                case "symbol":
                  if (/[>~+]/.test(token.text)) {
                    selectorState = SELECTOR_STATES.null;
                    selector += token.text;
                  } else if (token.text == ",") {
                    selectorState = SELECTOR_STATES.null;
                    selectors.push(selector);
                    selector = "";
                  } else if (token.text == ":") {
                    selectorState = SELECTOR_STATES.pseudo;
                    selector += ":";
                    if (cursor > tokIndex)
                      break;

                    token = tokens[cursor++];
                    switch(token.tokenType) {
                      case "function":
                        if (token.text == "not") {
                          selectorBeforeNot = selector;
                          selector = "";
                          scopeStack.push("(");
                        } else {
                          selector += token.text + "(";
                        }
                        selectorState = SELECTOR_STATES.null;
                        break;

                      case "ident":
                        selector += token.text;
                        break;
                    }
                  } else if (token.text == "[") {
                    selectorState = SELECTOR_STATES.attribute;
                    scopeStack.push("[");
                    selector += "[";
                  }
                  break;

                case "whitespace":
                  selectorState = SELECTOR_STATES.null;
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.attribute:
              switch(token.tokenType) {
                case "symbol":
                  if (/[~|^$*]/.test(token.text)) {
                    selector += token.text;
                    token = tokens[cursor++];
                  } else if (token.text == "=") {
                    selectorState = SELECTOR_STATES.value;
                    selector += token.text;
                  } else if (token.text == "]") {
                    if (peek(scopeStack) == "[")
                      scopeStack.pop();

                    selectorState = SELECTOR_STATES.null;
                    selector += "]";
                  }
                  break;

                case "ident":
                case "string":
                  selector += token.text;
                  break;

                case "whitespace":
                  selector && (selector += " ");
                  break;
              }
              break;

            case SELECTOR_STATES.value:
              switch(token.tokenType) {
                case "string":
                case "ident":
                  selector += token.text;
                  break;

                case "symbol":
                  if (token.text == "]") {
                    if (peek(scopeStack) == "[")
                      scopeStack.pop();

                    selectorState = SELECTOR_STATES.null;
                    selector += "]";
                  }
                  break;

                case "whitespace":
                  selector && (selector += " ");
                  break;
              }
              break;
          }
          break;

        case CSS_STATES.null:
          
          
          switch(token.tokenType) {
            case "hash":
            case "id":
              selectorState = SELECTOR_STATES.id;
              selector = "#" + token.text;
              _state = CSS_STATES.selector;
              break;

            case "ident":
              selectorState = SELECTOR_STATES.tag;
              selector = token.text;
              _state = CSS_STATES.selector;
              break;

            case "symbol":
              if (token.text == ".") {
                selectorState = SELECTOR_STATES.class;
                selector = ".";
                _state = CSS_STATES.selector;
                if (cursor <= tokIndex &&
                    tokens[cursor].tokenType == "ident") {
                  token = tokens[cursor++];
                  selector += token.text;
                }
              } else if (token.text == "#") {
                selectorState = SELECTOR_STATES.id;
                selector = "#";
                _state = CSS_STATES.selector;
              } else if (token.text == "*") {
                selectorState = SELECTOR_STATES.tag;
                selector = "*";
                _state = CSS_STATES.selector;
              } else if (token.text == ":") {
                _state = CSS_STATES.selector;
                selectorState = SELECTOR_STATES.pseudo;
                selector += ":";
                if (cursor > tokIndex)
                  break;

                token = tokens[cursor++];
                switch(token.tokenType) {
                  case "function":
                    if (token.text == "not") {
                      selectorBeforeNot = selector;
                      selector = "";
                      scopeStack.push("(");
                    } else {
                      selector += token.text + "(";
                    }
                    selectorState = SELECTOR_STATES.null;
                    break;

                  case "ident":
                    selector += token.text;
                    break;
                }
              } else if (token.text == "[") {
                _state = CSS_STATES.selector;
                selectorState = SELECTOR_STATES.attribute;
                scopeStack.push("[");
                selector += "[";
              } else if (token.text == "}") {
                if (peek(scopeStack) == "@m")
                  scopeStack.pop();
              }
              break;

            case "at":
              _state = token.text.startsWith("m") ? CSS_STATES.media
                                                   : CSS_STATES.keyframes;
              break;
          }
          break;

        case CSS_STATES.media:
          
          
          if (token.tokenType == "symbol" && token.text == "{") {
            scopeStack.push("@m");
            _state = CSS_STATES.null;
          }
          break;

        case CSS_STATES.keyframes:
          
          
          if (token.tokenType == "symbol" && token.text == "{") {
            scopeStack.push("@k");
            _state = CSS_STATES.frame;
          }
          break;

        case CSS_STATES.frame:
          
          
          if (token.tokenType == "symbol") {
            if (token.text == "{") {
              scopeStack.push("f");
              _state = CSS_STATES.property;
            } else if (token.text == "}") {
              if (peek(scopeStack) == "@k")
                scopeStack.pop();

              _state = CSS_STATES.null;
            }
          }
          break;
      }
      if (_state == CSS_STATES.null) {
        if (this.nullStates.length == 0) {
          this.nullStates.push([token.loc.end.line, token.loc.end.column,
                                [...scopeStack]]);
          continue;
        }
        let tokenLine = token.loc.end.line;
        let tokenCh = token.loc.end.column;
        if (tokenLine == 0)
          continue;
        if (matchedStateIndex > -1)
          tokenLine += this.nullStates[matchedStateIndex][0];
        this.nullStates.push([tokenLine, tokenCh, [...scopeStack]]);
      }
    }
    this.state = _state;
    this.propertyName = _state == CSS_STATES.value ? propertyName : null;
    this.selectorState = _state == CSS_STATES.selector ? selectorState : null;
    this.selectorBeforeNot = selectorBeforeNot == null ? null: selectorBeforeNot;
    if (token) {
      selector = selector.slice(0, selector.length + token.loc.end.column - ch);
      this.selector = selector;
    }
    else {
      this.selector = "";
    }
    this.selectors = selectors;

    if (token && token.tokenType != "whitespace") {
      let text;
      if (token.tokenType == "dimension" || !token.text)
        text = source.substring(token.startOffset, token.endOffset);
      else
        text = token.text;
      this.completing = (text.slice(0, ch - token.loc.start.column)
                         .replace(/^[.#]$/, ""));
    } else {
      this.completing = "";
    }
    
    
    if (this.completing == ":" && _state == CSS_STATES.value)
      this.completing = "";

    
    if (token && tokens[cursor - 2] && tokens[cursor - 2].text == "!" &&
        this.completing == "important".slice(0, this.completing.length)) {
      this.completing = "!" + this.completing;
    }
    return _state;
  },

  



  suggestSelectors: function () {
    let walker = this.walker;
    if (!walker)
      return Promise.resolve([]);

    let query = this.selector;
    
    
    switch(this.selectorState) {
      case SELECTOR_STATES.null:
        if (this.completing === ",") {
          return Promise.resolve([]);
        }

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
    for (let [value, count, state] of result) {
      switch(this.selectorState) {
        case SELECTOR_STATES.id:
        case SELECTOR_STATES.class:
        case SELECTOR_STATES.pseudo:
          if (/^[.:#]$/.test(this.completing)) {
            value = query.slice(0, query.length - this.completing.length) +
                       value;
          } else {
            value = query.slice(0, query.length - this.completing.length - 1) +
                       value;
          }
          break;

        case SELECTOR_STATES.tag:
          value = query.slice(0, query.length - this.completing.length) +
                     value;
          break;

        case SELECTOR_STATES.null:
          value = query + value;
          break;

        default:
         value = query.slice(0, query.length - this.completing.length) +
                    value;
      }

      let item = {
        label: value,
        preLabel: query,
        text: value,
        score: count
      };

      
      
      if (this.selectorState === SELECTOR_STATES.tag &&
          state === SELECTOR_STATES.class) {
        item.preLabel = "." + item.preLabel;
      }
      if (this.selectorState === SELECTOR_STATES.tag &&
          state === SELECTOR_STATES.id) {
        item.preLabel = "#" + item.preLabel;
      }

      completion.push(item);

      if (completion.length > this.maxEntries - 1)
        break;
    }
    return completion;
  },

  




  completeProperties: function(startProp) {
    let finalList = [];
    if (!startProp)
      return Promise.resolve(finalList);

    let length = propertyNames.length;
    let i = 0, count = 0;
    for (; i < length && count < this.maxEntries; i++) {
      if (propertyNames[i].startsWith(startProp)) {
        count++;
        let propName = propertyNames[i];
        finalList.push({
          preLabel: startProp,
          label: propName,
          text: propName + ": "
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
    
    
    if (!startValue)
      list.splice(0, 1);

    let length = list.length;
    let i = 0, count = 0;
    for (; i < length && count < this.maxEntries; i++) {
      if (list[i].startsWith(startValue)) {
        count++;
        let value = list[i];
        finalList.push({
          preLabel: startValue,
          label: value,
          text: value
        });
      } else if (list[i] > startValue) {
        
        break;
      }
    }
    return Promise.resolve(finalList);
  },

  










  findNearestNullState: function(line) {
    let arr = this.nullStates;
    let high = arr.length - 1;
    let low = 0;
    let target = 0;

    if (high < 0)
      return -1;
    if (arr[high][0] <= line)
      return high;
    if (arr[low][0] > line)
      return -1;

    while (high > low) {
      if (arr[low][0] <= line && arr[low [0]+ 1] > line)
        return low;
      if (arr[high][0] > line && arr[high - 1][0] <= line)
        return high - 1;

      target = (((line - arr[low][0]) / (arr[high][0] - arr[low][0])) *
                (high - low)) | 0;

      if (arr[target][0] <= line && arr[target + 1][0] > line) {
        return target;
      } else if (line > arr[target][0]) {
        low = target + 1;
        high--;
      } else {
        high = target - 1;
        low++;
      }
    }

    return -1;
  },

  


  invalidateCache: function(line) {
    this.nullStates.length = this.findNearestNullState(line) + 1;
  },

  

























  getInfoAt: function(source, caret) {
    
    function limit(source, {line, ch}) {
      line++;
      let list = source.split("\n");
      if (list.length < line)
        return source;
      if (line == 1)
        return list[0].slice(0, ch);
      return [...list.slice(0, line - 1), list[line - 1].slice(0, ch)].join("\n");
    }

    
    let state = this.resolveState(limit(source, caret), caret);
    let propertyName = this.propertyName;
    let {line, ch} = caret;
    let sourceArray = source.split("\n");
    let limitedSource = limit(source, caret);

    







    let traverseForward = check => {
      let location;
      
      do {
        let lineText = sourceArray[line];
        if (line == caret.line)
          lineText = lineText.substring(caret.ch);

        let prevToken = undefined;
        let tokens = cssTokenizer(lineText);
        let found = false;
        let ech = line == caret.line ? caret.ch : 0;
        for (let token of tokens) {
          
          if (lineText.trim() == "") {
            limitedSource += lineText;
          } else {
            limitedSource += sourceArray[line]
                              .substring(ech + token.startOffset,
                                         ech + token.endOffset);
          }

          
          if (token.tokenType == "whitespace") {
            prevToken = token;
            continue;
          }

          let state = this.resolveState(limitedSource, {
            line: line,
            ch: token.endOffset + ech
          });
          if (check(state)) {
            if (prevToken && prevToken.tokenType == "whitespace")
              token = prevToken;
            location = {
              line: line,
              ch: token.startOffset + ech
            };
            found = true;
            break;
          }
          prevToken = token;
        }
        limitedSource += "\n";
        if (found)
          break;
      } while (line++ < sourceArray.length);
      return location;
    };

    









    let traverseBackwards = (check, isValue) => {
      let location;
      
      do {
        let lineText = sourceArray[line];
        if (line == caret.line)
          lineText = lineText.substring(0, caret.ch);

        let tokens = Array.from(cssTokenizer(lineText));
        let found = false;
        let ech = 0;
        for (let i = tokens.length - 1; i >= 0; i--) {
          let token = tokens[i];
          
          if (lineText.trim() == "") {
            limitedSource = limitedSource.slice(0, -1 * lineText.length);
          } else {
            let length = token.endOffset - token.startOffset;
            limitedSource = limitedSource.slice(0, -1 * length);
          }

          
          if (token.tokenType == "whitespace")
            continue;

          let state = this.resolveState(limitedSource, {
            line: line,
            ch: token.startOffset
          });
          if (check(state)) {
            if (tokens[i + 1] && tokens[i + 1].tokenType == "whitespace")
              token = tokens[i + 1];
            location = {
              line: line,
              ch: isValue ? token.endOffset: token.startOffset
            };
            found = true;
            break;
          }
        }
        limitedSource = limitedSource.slice(0, -1);
        if (found)
          break;
      } while (line-- >= 0);
      return location;
    };

    if (state == CSS_STATES.selector) {
      
      
      
      
      let start = traverseBackwards(state => {
        return (state != CSS_STATES.selector ||
               (this.selector == "" && this.selectorBeforeNot == null));
      });

      line = caret.line;
      limitedSource = limit(source, caret);
      
      let end = traverseForward(state => {
        return (state != CSS_STATES.selector ||
               (this.selector == "" && this.selectorBeforeNot == null));
      });

      
      let selector = source.split("\n").slice(start.line, end.line + 1);
      selector[selector.length - 1] =
        selector[selector.length - 1].substring(0, end.ch);
      selector[0] = selector[0].substring(start.ch);
      selector = selector.join("\n");
      return {
        state: state,
        selector: selector,
        loc: {
          start: start,
          end: end
        }
      };
    }
    else if (state == CSS_STATES.property) {
      
      let tokens = cssTokenizer(sourceArray[line]);
      for (let token of tokens) {
        
        
        if (token.startOffset <= ch && token.endOffset >= ch) {
          return {
            state: state,
            propertyName: token.text,
            selectors: this.selectors,
            loc: {
              start: {
                line: line,
                ch: token.startOffset
              },
              end: {
                line: line,
                ch: token.endOffset
              }
            }
          };
        }
      }
    }
    else if (state == CSS_STATES.value) {
      
      
      let start = traverseBackwards(state => state != CSS_STATES.value, true);

      line = caret.line;
      limitedSource = limit(source, caret);
      let end = traverseForward(state => state != CSS_STATES.value);

      let value = source.split("\n").slice(start.line, end.line + 1);
      value[value.length - 1] = value[value.length - 1].substring(0, end.ch);
      value[0] = value[0].substring(start.ch);
      value = value.join("\n");
      return {
        state: state,
        propertyName: propertyName,
        selectors: this.selectors,
        value: value,
        loc: {
          start: start,
          end: end
        }
      };
    }
    return null;
  }
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
