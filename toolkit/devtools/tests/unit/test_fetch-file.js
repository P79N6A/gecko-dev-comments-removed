


"use strict";



const { FileUtils } = Cu.import("resource://gre/modules/FileUtils.jsm");
const { OS } = Cu.import("resource://gre/modules/osfile.jsm", {});

const TEST_CONTENT = "let a = 1 + 1";






add_task(function* test_arrow_urls() {
  let { path } = createTemporaryFile(".js");
  let url = "resource://gre/modules/XPIProvider.jsm -> file://" + path;

  yield OS.File.writeAtomic(path, TEST_CONTENT, { encoding: "utf-8" });
  let { content }Â = yield DevToolsUtils.fetch(url);

  deepEqual(content, TEST_CONTENT, "The file contents were correctly read.");
});




add_task(function* test_empty() {
  let { path } = createTemporaryFile();
  let { content } = yield DevToolsUtils.fetch("file://" + path);
  deepEqual(content, "", "The empty file was read correctly.");
});




add_task(function* test_missing() {
  yield DevToolsUtils.fetch("file:///file/not/found.right").then(result => {
    do_print(result);
    ok(false, "Fetch resolved unexpectedly when the file was not found.");
  }, () => {
    ok(true, "Fetch rejected as expected because the file was not found.");
  });
});




add_task(function* test_normal() {
  let { path } = createTemporaryFile(".js");

  yield OS.File.writeAtomic(path, TEST_CONTENT, { encoding: "utf-8" });

  let { content } = yield DevToolsUtils.fetch("file://" + path);
  deepEqual(content, TEST_CONTENT, "The file contents were correctly read.");
});




add_task(function* test_schemeless_files() {
  let { path } = createTemporaryFile();

  yield OS.File.writeAtomic(path, TEST_CONTENT, { encoding: "utf-8" });

  let { content } = yield DevToolsUtils.fetch(path);
  deepEqual(content, TEST_CONTENT, "The content was correct.");
});




function createTemporaryFile(extension) {
  let name = "test_fetch-file-" + Math.random() + (extension || "");
  let file = FileUtils.getFile("TmpD", [name]);
  file.create(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("0755", 8));

  do_register_cleanup(() => {
    file.remove(false);
  });

  return file;
}
