


console.log("Initializing worker.");

self.onmessage = e => {
  console.log("Starting test.");
  try {
    const { filePath } = e.data;

    ok(typeof ChromeUtils === "undefined",
       "Should not have access to ChromeUtils in a worker.");
    ok(ThreadSafeChromeUtils,
       "Should have access to ThreadSafeChromeUtils in a worker.");
    ok(HeapSnapshot,
       "Should have access to HeapSnapshot in a worker.");

    ThreadSafeChromeUtils.saveHeapSnapshot(filePath, { globals: [this] });
    ok(true, "Should be able to save a snapshot.");

    const snapshot = ThreadSafeChromeUtils.readHeapSnapshot(filePath);
    ok(snapshot, "Should be able to read a heap snapshot");
    ok(snapshot instanceof HeapSnapshot, "Should be an instanceof HeapSnapshot");
  } catch (e) {
    ok(false, "Unexpected error inside worker:\n" + e.toString() + "\n" + e.stack);
  } finally {
    done();
  }
};


function ok(val, msg) {
  console.log("ok(" + !!val + ", \"" + msg + "\")");
  self.postMessage({
    type: "assertion",
    passed: !!val,
    msg,
    stack: Error().stack
  });
}


function done() {
  console.log("done()");
  self.postMessage({
    type: "done"
  });
}
