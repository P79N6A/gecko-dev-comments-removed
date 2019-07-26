load(libdir + "asserts.js");

function xmlAllowed() {
  return (typeof options == "function") &&
         (options() + "").contains("allow_xml");
}


var valid_strict_funs = [
  
  function () {
    "use strict"
    ;
  },
  function () {
    "use strict"
  },
  
  function () { "use strict"; },
  function () { "use strict" },
];

for (var f of valid_strict_funs) {
  assertThrowsInstanceOf(function() { f.caller }, TypeError);
}


var binary_ops = [
  "||", "&&",
  "|", "^", "&",
  "==", "!=", "===", "!==",
  "<", "<=", ">", ">=", "in", "instanceof",
  "<<", ">>", ">>>",
  "+", "-",
  "*", "/", "%",
];

var invalid_strict_funs = [
  function () {
    "use strict"
    , "not";
  },
  function () {
    "use strict"
    ? 1 : 0;
  },
  function () {
    "use strict"
    .length;
  },
  function () {
    "use strict"
    [0];
  },
  function () {
    "use strict"
    ();
  },
  ...(xmlAllowed() ? [Function("'use strict'\n..not")] : []),
  ...[Function("'use strict'\n " + op + " 'not'") for (op of binary_ops)],
];

for (var f of invalid_strict_funs) {
  f.caller;
}


var assignment_ops = [
   "=", "+=", "-=",
  "|=", "^=", "&=",
  "<<=", ">>=", ">>>=",
  "*=", "/=", "%=",
];

var invalid_strict_funs_referror = [
  ...[("'use strict'\n " + op + " 'not'") for (op of assignment_ops)],
];



for (var f of invalid_strict_funs_referror) {
  assertThrowsInstanceOf(function() { Function(f) }, ReferenceError);
}
