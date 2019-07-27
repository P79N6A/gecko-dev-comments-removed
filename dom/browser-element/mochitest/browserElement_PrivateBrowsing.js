



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
  return new Promise((aResolve) => {
    var iframe = createFrame(aIsPrivate);
    document.body.appendChild(iframe);

    iframe.addEventListener("mozbrowsershowmodalprompt", function(e) {
      is(e.detail.message, aExpected, "Checking localstorage");
      aResolve();
    });

    iframe.src = "file_browserElement_PrivateBrowsing.html";
  });
}

function runTest() {
  
  
  
  
  createTest(false, "EMPTY")
  .then(() => { return createTest(false, "bar"); })
  .then(() => { return createTest(true, "EMPTY"); })
  .then(SimpleTest.finish);
}

addEventListener("testready", runTest);
