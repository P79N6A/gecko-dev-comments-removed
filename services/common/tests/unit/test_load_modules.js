


const modules = [
  "async.js",
  "log4moz.js",
  "preferences.js",
  "rest.js",
  "storageservice.js",
  "stringbundle.js",
  "tokenserverclient.js",
  "utils.js",
];

const test_modules = [
  "aitcserver.js",
  "logging.js",
  "storageserver.js",
];

function run_test() {
  for each (let m in modules) {
    let resource = "resource://services-common/" + m;
    Components.utils.import(resource, {});
  }

  for each (let m in test_modules) {
    let resource = "resource://testing-common/services-common/" + m;
    Components.utils.import(resource, {});
  }
}
