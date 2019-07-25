



function test() {
  const URL_1 = "mozilla.org";
  const URL_2 = "mozilla.com";

  let openLocationLastURL = getLocationModule();
  let privateBrowsingService =
    Cc["@mozilla.org/privatebrowsing;1"].
      getService(Ci.nsIPrivateBrowsingService);

  function clearHistory() {
    Services.obs.notifyObservers(null, "browser:purge-session-history", "");
  }
  function testURL(aTestNumber, aValue) {
    is(openLocationLastURL.value, aValue,
       "Test: " + aTestNumber + ": Validate last url value.");
  }

  
  is(typeof openLocationLastURL, "object", "Validate type of last url.");
  openLocationLastURL.reset();
  testURL(1, "");

  
  openLocationLastURL.value = URL_1;
  testURL(2, URL_1);
  openLocationLastURL.value = "";
  testURL(3, "");
  openLocationLastURL.value = URL_2;
  testURL(4, URL_2);
  clearHistory();
  testURL(5, "");

  
  openLocationLastURL.value = URL_2;
  privateBrowsingService.privateBrowsingEnabled = true;
  testURL(6, "");
  privateBrowsingService.privateBrowsingEnabled = false;
  testURL(7, URL_2);
  privateBrowsingService.privateBrowsingEnabled = true;
  openLocationLastURL.value = URL_1;
  testURL(8, URL_1);
  privateBrowsingService.privateBrowsingEnabled = false;
  testURL(9, URL_2);
  privateBrowsingService.privateBrowsingEnabled = true;
  openLocationLastURL.value = URL_1;
  testURL(10, URL_1);

  
  clearHistory();
  testURL(11, "");
  privateBrowsingService.privateBrowsingEnabled = false;
  testURL(12, "");
}

function getLocationModule() {
  let openLocationModule = {};
  Cu.import("resource:///modules/openLocationLastURL.jsm", openLocationModule);
  return new openLocationModule.OpenLocationLastURL(window);
}
