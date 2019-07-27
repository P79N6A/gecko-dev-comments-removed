


'use strict';

const ContentWorker = require('../content/worker').Worker;

function Worker(options, window) {
  options.window = window;

  let worker = ContentWorker(options);
  worker.once("detach", function detach() {
    worker.destroy();
  });
  return worker;
}
exports.Worker = Worker;