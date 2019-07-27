









































(function(exports) {
  "use strict";

  if (exports.require) {
    
    return;
  }

  
  let require = (function() {

    





    let modules = new Map();

    




    Object.defineProperty(Error.prototype, "moduleStack",
    {
      get: function() {
        return this.stack;
      }
    });
    




    Object.defineProperty(Error.prototype, "moduleName",
    {
      get: function() {
        let match = this.stack.match(/\@(.*):.*:/);
        if (match) {
          return match[1];
        }
        return "(unknown module)";
      }
    });

    





    return function require(path) {
      if (typeof path != "string" || path.indexOf("://") == -1) {
        throw new TypeError("The argument to require() must be a string uri, got " + path);
      }
      
      let uri;
      if (path.lastIndexOf(".") <= path.lastIndexOf("/")) {
        uri = path + ".js";
      } else {
        uri = path;
      }

      
      let exports = Object.create(null);

      
      let module = {
        id: path,
        uri: uri,
        exports: exports
      };

      
      
      if (modules.has(path)) {
        return modules.get(path).exports;
      }
      modules.set(path, module);

      let name = ":" + path;
      let objectURL;
      try {
        
        let xhr = new XMLHttpRequest();
        xhr.open("GET", uri, false);
        xhr.responseType = "text";
        xhr.send();


        let source = xhr.responseText;
        if (source == "") {
          
          throw new Error("Could not find module " + path);
        }
        let code = new Function("exports", "require", "module",
          source + "\n//# sourceURL=" + uri + "\n"
        );
        code(exports, require, module);
      } catch (ex) {
        
        
        modules.delete(path);
        throw ex;
      }

      Object.freeze(module.exports);
      Object.freeze(module);
      return module.exports;
    };
  })();

  Object.freeze(require);

  Object.defineProperty(exports, "require", {
    value: require,
    enumerable: true,
    configurable: false
  });
})(this);
