



function test() {
  waitForExplicitFinish();
  const URL_1 = "mozilla.org";
  const URL_2 = "mozilla.com";

  function testURL(aOpenLocation, aTestNumber, aValue) {
    is(aOpenLocation.value, aValue,
       "Test: " + aTestNumber + ": Validate last url value.");
  }

  whenNewWindowLoaded({private: false}, function(normalWindow) {
    whenNewWindowLoaded({private: true}, function(privateWindow) {
      let openLocationLastURL = getLocationModule(normalWindow);
      let openLocationLastURLPB = getLocationModule(privateWindow);

      
      is(typeof openLocationLastURL, "object", "Validate Normal type of last url.");
      is(typeof openLocationLastURLPB, "object", "Validate PB type of last url.");
      openLocationLastURL.reset();
      openLocationLastURLPB.reset();
      testURL(openLocationLastURL, 1, "");

      
      openLocationLastURL.value = URL_1;
      testURL(openLocationLastURL, 2, URL_1);
      openLocationLastURL.value = "";
      testURL(openLocationLastURL, 3, "");
      openLocationLastURL.value = URL_2;
      testURL(openLocationLastURL, 4, URL_2);
      clearHistory();
      testURL(openLocationLastURL, 5, "");

      
      openLocationLastURL.value = URL_2;
      testURL(openLocationLastURLPB, 6, "");
      testURL(openLocationLastURL, 7, URL_2);
      openLocationLastURLPB.value = URL_1;
      testURL(openLocationLastURLPB, 8, URL_1);
      testURL(openLocationLastURL, 9, URL_2);
      openLocationLastURLPB.value = URL_1;
      testURL(openLocationLastURLPB, 10, URL_1);

      
      clearHistory();
      testURL(openLocationLastURLPB, 11, "");
      testURL(openLocationLastURL, 12, "");

      privateWindow.close();
      normalWindow.close();
      finish();
    });
  });
}

function getLocationModule(aWindow) {
  let openLocationModule = {};
  Cu.import("resource:///modules/openLocationLastURL.jsm", openLocationModule);
  return new openLocationModule.OpenLocationLastURL(aWindow);
}