



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

function createTest(aIsPrivate, aExpected, aClearStorage) {
  info("createTest " + aIsPrivate + " " + aExpected);
  return new Promise(function(resolve, reject) {
    var iframe = createFrame(aIsPrivate);
    document.body.appendChild(iframe);

    iframe.addEventListener("mozbrowsershowmodalprompt", function(e) {
      is(e.detail.message, aExpected, "Checking localstorage");
      resolve();
    });

    var src = "file_browserElement_PrivateBrowsing.html";
    iframe.src = aClearStorage ? src + "?clear=true" : src;

  });
}

function runTest() {
  
  
  
  
  createTest(false, "CLEAR", true)
  .then(() => { return createTest(false, "EMPTY", false); })
  .then(() => { return createTest(false, "bar", false); })
  .then(() => { return createTest(true, "EMPTY", false); })
  .then(SimpleTest.finish);
}

addEventListener("testready", runTest);
