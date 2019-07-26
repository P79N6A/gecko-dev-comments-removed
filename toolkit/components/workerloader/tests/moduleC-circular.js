







exports.enteredC = true;

let D = require("chrome://mochitests/content/chrome/toolkit/components/workerloader/tests/moduleD-circular.js");



exports.copiedFromD = JSON.parse(JSON.stringify(D));

exports.exportedFromD = D;
exports.finishedC = true;