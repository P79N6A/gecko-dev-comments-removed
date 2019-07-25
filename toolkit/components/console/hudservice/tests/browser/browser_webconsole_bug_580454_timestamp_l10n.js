











Cu.import("resource:///modules/HUDService.jsm");

const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testTimestamp, false);

  function testTimestamp()
  {
    browser.removeEventListener("DOMContentLoaded", testTimestamp, false);
    const TEST_TIMESTAMP = 12345678;
    let date = new Date(TEST_TIMESTAMP);
    let localizedString = ConsoleUtils.timestampString(TEST_TIMESTAMP);
    isnot(localizedString.indexOf(date.getHours()), -1, "the localized " +
          "timestamp contains the hours");
    isnot(localizedString.indexOf(date.getMinutes()), -1, "the localized " +
          "timestamp contains the minutes");
    isnot(localizedString.indexOf(date.getSeconds()), -1, "the localized " +
          "timestamp contains the seconds");
    isnot(localizedString.indexOf(date.getMilliseconds()), -1, "the localized " +
          "timestamp contains the milliseconds");
    finishTest();
  }
}

