





exports.enteredD = true;
let C = require("chrome://mochitests/content/chrome/toolkit/components/workerloader/tests/moduleC-circular.js");
exports.copiedFromC = JSON.parse(JSON.stringify(C));
exports.exportedFromC = C;
exports.finishedD = true;