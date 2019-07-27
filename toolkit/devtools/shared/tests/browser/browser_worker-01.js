





const WORKER_URL = "resource:///modules/devtools/GraphsWorker.js";

const count = 100000;
const WORKER_DATA = (function () {
  let timestamps = [];
  for (let i = 0; i < count; i++) {
    timestamps.push(i);
  }
  return timestamps;
})();
const INTERVAL = 100;
const DURATION = 1000;

add_task(function*() {
  

  yield testWorker("JSM", () => Cu.import("resource://gre/modules/devtools/shared/worker.js", {}));
  yield testWorker("CommonJS", () => require("devtools/toolkit/shared/worker"));
});

function *testWorker (context, workerFactory) {
  let { DevToolsWorker, workerify } = workerFactory();
  let worker = new DevToolsWorker(WORKER_URL);
  let results = yield worker.performTask("plotTimestampsGraph", {
    timestamps: WORKER_DATA,
    interval: INTERVAL,
    duration: DURATION
  });

  ok(results.plottedData.length,
    `worker should have returned an object with array properties in ${context}`);

  let fn = workerify(function (x) { return x * x });
  is((yield fn(5)), 25, `workerify works in ${context}`);
  fn.destroy();

  worker.destroy();
}
