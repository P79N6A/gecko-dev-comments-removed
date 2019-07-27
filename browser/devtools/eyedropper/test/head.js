



Services.scriptloader.loadSubScript("chrome://mochitests/content/browser/browser/devtools/framework/test/shared-head.js", this);
Services.scriptloader.loadSubScript(TEST_DIR + "../../../commandline/test/helpers.js", this);

const { Eyedropper, EyedropperManager } = devtools.require("devtools/eyedropper/eyedropper");

function waitForClipboard(setup, expected) {
  let deferred = promise.defer();
  SimpleTest.waitForClipboard(expected, setup, deferred.resolve, deferred.reject);
  return deferred.promise;
}

function dropperStarted(dropper) {
  if (dropper.isStarted) {
    return promise.resolve();
  }
  return dropper.once("started");
}

function dropperLoaded(dropper) {
  if (dropper.loaded) {
    return promise.resolve();
  }
  return dropper.once("load");
}
