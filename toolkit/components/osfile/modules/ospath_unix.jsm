
















"use strict";



if (typeof Components != "undefined") {
  
  
  
  this.exports = {};
} else if (typeof "module" == "undefined" || typeof "exports" == "undefined") {
  throw new Error("Please load this module using require()");
}

let EXPORTED_SYMBOLS = [
  "basename",
  "dirname",
  "join",
  "normalize",
  "split"
];





let basename = function(path) {
  return path.slice(path.lastIndexOf("/") + 1);
};
exports.basename = basename;









let dirname = function(path) {
  let index = path.lastIndexOf("/");
  if (index == -1) {
    return ".";
  }
  while (index >= 0 && path[index] == "/") {
    --index;
  }
  return path.slice(0, index + 1);
};
exports.dirname = dirname;












let join = function(path ) {
  
  let paths = [];
  for each(let i in arguments) {
    if (i.length != 0 && i[0] == "/") {
      paths = [i];
    } else {
      paths.push(i);
    }
  }
  return paths.join("/");
};
exports.join = join;




let normalize = function(path) {
  let stack = [];
  let absolute;
  if (path.length >= 0 && path[0] == "/") {
    absolute = true;
  } else {
    absolute = false;
  }
  path.split("/").forEach(function(v) {
    switch (v) {
    case "":  case ".":
      break;
    case "..":
      if (stack.length == 0) {
        if (absolute) {
          throw new Error("Path is ill-formed: attempting to go past root");
        } else {
          stack.push("..");
        }
      } else {
        if (stack[stack.length - 1] == "..") {
          stack.push("..");
        } else {
          stack.pop();
        }
      }
      break;
    default:
      stack.push(v);
    }
  });
  let string = stack.join("/");
  return absolute ? "/" + string : string;
};
exports.normalize = normalize;












let split = function(path) {
  return {
    absolute: path.length && path[0] == "/",
    components: path.split("/")
  };
};
exports.split = split;


if (typeof Components != "undefined") {
  this.EXPORTED_SYMBOLS = EXPORTED_SYMBOLS;
  for (let symbol of EXPORTED_SYMBOLS) {
    this[symbol] = exports[symbol];
  }
}
