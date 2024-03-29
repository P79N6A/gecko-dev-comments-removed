


"use strict";





let { Loader } = Components.utils.import('resource://gre/modules/commonjs/toolkit/loader.js', {});

function run_test() {
  run_next_test();
}


add_task(function*() {
  let dataDir = Services.io.newFileURI(do_get_file("test_loader/", true)).spec + "/";
  let loader = Loader.Loader({
    paths: {'': dataDir }
  });

  let require = Loader.Require(loader, Loader.Module('module_test_loader', 'foo'));
  do_print("Require is ready");
  try {
    require('module_test_loader');
  } catch (error) {
    dump('Bootstrap error: ' +
         (error.message ? error.message : String(error)) + '\n' +
         (error.stack || error.fileName + ': ' + error.lineNumber) + '\n');

    throw error;
  }

  do_print("Require has worked");
});

