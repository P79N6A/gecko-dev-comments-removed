




















































EXPORTED_SYMBOLS = ["JSON"];









var JSON = {
  









  toString: function JSON_toString(aJSObject, aKeysToDrop) {
    
    const charMap = { "\b": "\\b", "\t": "\\t", "\n": "\\n", "\f": "\\f",
                      "\r": "\\r", '"': '\\"', "\\": "\\\\" };
    
    
    var pieces = [];
    
    
    
    function append_piece(aObj) {
      if (typeof aObj == "boolean") {
        pieces.push(aObj ? "true" : "false");
      }
      else if (typeof aObj == "number" && isFinite(aObj)) {
        
        pieces.push(aObj.toString());
      }
      else if (typeof aObj == "string") {
        aObj = aObj.replace(/[\\"\x00-\x1F\u0080-\uFFFF]/g, function($0) {
          
          
          return charMap[$0] ||
            "\\u" + ("0000" + $0.charCodeAt(0).toString(16)).slice(-4);
        });
        pieces.push('"' + aObj + '"')
      }
      else if (aObj === null) {
        pieces.push("null");
      }
      
      
      else if (aObj instanceof Array ||
               typeof aObj == "object" && "length" in aObj &&
               (aObj.length === 0 || aObj[aObj.length - 1] !== undefined)) {
        pieces.push("[");
        for (var i = 0; i < aObj.length; i++) {
          append_piece(aObj[i]);
          pieces.push(",");
        }
        if (pieces[pieces.length - 1] == ",")
          pieces.pop(); 
        pieces.push("]");
      }
      else if (typeof aObj == "object") {
        pieces.push("{");
        for (var key in aObj) {
          
          
          
          if (aKeysToDrop && aKeysToDrop.indexOf(key) != -1)
            continue;
          
          append_piece(key.toString());
          pieces.push(":");
          append_piece(aObj[key]);
          pieces.push(",");
        }
        if (pieces[pieces.length - 1] == ",")
          pieces.pop(); 
        pieces.push("}");
      }
      else {
        throw new TypeError("No JSON representation for this object!");
      }
    }
    append_piece(aJSObject);
    
    return pieces.join("");
  },

  





  fromString: function JSON_fromString(aJSONString) {
    if (!this.isMostlyHarmless(aJSONString))
      throw new SyntaxError("No valid JSON string!");
    
    var s = new Components.utils.Sandbox("about:blank");
    return Components.utils.evalInSandbox("(" + aJSONString + ")", s);
  },

  







  isMostlyHarmless: function JSON_isMostlyHarmless(aString) {
    const maybeHarmful = /[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/;
    const jsonStrings = /"(\\.|[^"\\\n\r])*"/g;
    
    return !maybeHarmful.test(aString.replace(jsonStrings, ""));
  }
};
