




const { addDebuggerToGlobal } = Cu.import("resource://gre/modules/jsdebugger.jsm", {});
var Debugger;
addDebuggerToGlobal(this);

function run_test() {
  const filePath = getFilePath("core-dump.tmp", true, true);
  ok(filePath, "Should get a file path");

  ChromeUtils.saveHeapSnapshot(filePath, { globals: [this] });
  ok(true, "Should be able to save a snapshot.");

  const snapshot = ChromeUtils.readHeapSnapshot(filePath);
  ok(snapshot, "Should be able to read a heap snapshot");
  ok(snapshot instanceof HeapSnapshot, "Should be an instanceof HeapSnapshot");

  do_test_finished();
}
