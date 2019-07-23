




















































var EXPORTED_SYMBOLS = ["JSONModule"];









var JSONModule = {
  









  toString: function JSON_toString(aJSObject, aKeysToDrop) {
    
    var pieces = [];
    
    
    
    function append_piece(aObj) {
      if (typeof aObj == "string") {
        aObj = aObj.replace(/[\\"\x00-\x1F\u0080-\uFFFF]/g, function($0) {
          
          
          switch ($0) {
          case "\b": return "\\b";
          case "\t": return "\\t";
          case "\n": return "\\n";
          case "\f": return "\\f";
          case "\r": return "\\r";
          case '"':  return '\\"';
          case "\\": return "\\\\";
          }
          return "\\u" + ("0000" + $0.charCodeAt(0).toString(16)).slice(-4);
        });
        pieces.push('"' + aObj + '"')
      }
      else if (typeof aObj == "boolean") {
        pieces.push(aObj ? "true" : "false");
      }
      else if (typeof aObj == "number" && isFinite(aObj)) {
        
        pieces.push(aObj.toString());
      }
      else if (aObj === null) {
        pieces.push("null");
      }
      
      
      else if (aObj instanceof Array ||
               typeof aObj == "object" && "length" in aObj &&
               (aObj.length === 0 || aObj[aObj.length - 1] !== undefined)) {
        pieces.push("[");
        for (var i = 0; i < aObj.length; i++) {
          arguments.callee(aObj[i]);
          pieces.push(",");
        }
        if (aObj.length > 0)
          pieces.pop(); 
        pieces.push("]");
      }
      else if (typeof aObj == "object") {
        pieces.push("{");
        for (var key in aObj) {
          
          
          
          if (aKeysToDrop && aKeysToDrop.indexOf(key) != -1)
            continue;
          
          arguments.callee(key.toString());
          pieces.push(":");
          arguments.callee(aObj[key]);
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
