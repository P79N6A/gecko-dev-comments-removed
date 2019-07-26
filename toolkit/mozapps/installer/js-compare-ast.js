













"use strict";

function ast(filename) {
  return JSON.stringify(Reflect.parse(snarf(filename), {loc: 0}));
}

if (scriptArgs.length !== 2) {
  throw "usage: js js-compare-ast.js FILE1.js FILE2.js";
}

let ast0 = ast(scriptArgs[0]);
let ast1 = ast(scriptArgs[1]);

quit(ast0 == ast1 ? 0 : 1);
