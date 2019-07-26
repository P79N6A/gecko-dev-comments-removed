


 "use strict";

const tmp = require("sdk/test/tmp-file");
const file = require("sdk/io/file");

const testFolderURL = module.uri.split('test-tmp-file.js')[0];

exports.testCreateFromString = function (assert) {
  let expectedContent = "foo";
  let path = tmp.createFromString(expectedContent);
  let content = file.read(path);
  assert.equal(content, expectedContent,
               "Temporary file contains the expected content");
}

exports.testCreateFromURL = function (assert) {
  let url = testFolderURL + "test-tmp-file.txt";
  let path = tmp.createFromURL(url);
  let content = file.read(path);
  assert.equal(content, "foo",
               "Temporary file contains the expected content");
}

require("sdk/test").run(exports);
