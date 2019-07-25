


const modules = [
  "async.js",
  "log4moz.js",
  "preferences.js",
  "rest.js",
  "stringbundle.js",
  "tokenserverclient.js",
  "utils.js",
];

const test_modules = [
  "storageserver.js",
];

function run_test() {
  for each (let m in modules) {
    let resource = "resource://services-common/" + m;
    Components.utils.import(resource, {});
  }

  
  





}
