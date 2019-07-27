






Components.utils.import("resource://testing-common/MockRegistrar.jsm");

"use strict";


let promptService = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPromptService]),
  confirmEx: function() {}
};
let prompt = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
  alert: function() {}
};



MockRegistrar.register("@mozilla.org/embedcomp/prompt-service;1", promptService);
MockRegistrar.register("@mozilla.org/prompter;1", prompt);



add_test(function init_search_service() {
  Services.search.init(function (status) {
    if (!Components.isSuccessCode(status))
      do_throw("Failed to initialize search service");

    run_next_test();
  });
});


add_test(function simple_callback_test() {
  let searchCallback = {
    onSuccess: function (engine) {
      do_check_true(!!engine);
      do_check_neq(engine.name, Services.search.defaultEngine.name);
      run_next_test();
    },
    onError: function (errorCode) {
      do_throw("search callback returned error: " + errorCode);
    }
  }
  Services.search.addEngine(gDataUrl + "engine.xml",
                            Ci.nsISearchEngine.DATA_XML,
                            null, false, searchCallback);
});


add_test(function duplicate_failure_test() {
  let searchCallback = {
    onSuccess: function (engine) {
      do_throw("this addition should not have succeeded");
    },
    onError: function (errorCode) {
      do_check_true(!!errorCode);
      do_check_eq(errorCode, Ci.nsISearchInstallCallback.ERROR_DUPLICATE_ENGINE);
      run_next_test();
    }
  }
  
  Services.search.addEngine(gDataUrl + "engine.xml",
                            Ci.nsISearchEngine.DATA_XML,
                            null, false, searchCallback);
});


add_test(function load_failure_test() {
  let searchCallback = {
    onSuccess: function (engine) {
      do_throw("this addition should not have succeeded");
    },
    onError: function (errorCode) {
      do_check_true(!!errorCode);
      do_check_eq(errorCode, Ci.nsISearchInstallCallback.ERROR_UNKNOWN_FAILURE);
      run_next_test();
    }
  }
  
  Services.search.addEngine("http://invalid/data/engine.xml",
                            Ci.nsISearchEngine.DATA_XML,
                            null, false, searchCallback);
});

function run_test() {
  updateAppInfo();
  useHttpServer();

  run_next_test();
}
