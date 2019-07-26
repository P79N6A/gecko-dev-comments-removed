



let B = require("chrome://mochitests/content/chrome/toolkit/components/workerloader/tests/moduleB-dependency.js");


if (Object.keys(exports).length) {
  throw new Error("exports should be empty, initially");
}


exports.A = true;
exports.importedFoo = B.foo;
