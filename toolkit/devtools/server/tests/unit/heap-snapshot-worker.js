


console.log("Initializing worker.");

self.onmessage = e => {
  console.log("Starting test.");
  try {
    const { filePath } = e.data;

    ok(ChromeUtils, "Should have access to ChromeUtils");
    ok(HeapSnapshot, "Should have access to HeapSnapshot");

    ChromeUtils.saveHeapSnapshot(filePath, { globals: [this] });
    ok(true, "Should be able to save a snapshot.");

    const snapshot = ChromeUtils.readHeapSnapshot(filePath);
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
