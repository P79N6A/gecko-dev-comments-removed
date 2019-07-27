



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function createFrame(aIsPrivate) {
  var iframe = document.createElement("iframe");
  SpecialPowers.wrap(iframe).mozbrowser = true;
  if (aIsPrivate) {
    iframe.setAttribute("mozprivatebrowsing", "true");
  }
  return iframe;
}

function createTest(aIsPrivate, aExpected, aNext) {
  info("createTest " + aIsPrivate + " " + aExpected);
  var deferred = Promise.defer();

  var iframe = createFrame(aIsPrivate);
  document.body.appendChild(iframe);

  iframe.addEventListener("mozbrowsershowmodalprompt", function(e) {
    is(e.detail.message, aExpected, "Checking localstorage");
    deferred.resolve();
  });

  iframe.src = "file_browserElement_PrivateBrowsing.html";

  return deferred.promise;
}

function runTest() {
  
  
  
  
  createTest(false, "EMPTY")
  .then(() => { return createTest(false, "bar"); })
  .then(() => { return createTest(true, "EMPTY"); })
  .then(SimpleTest.finish);
}

addEventListener("testready", runTest);
