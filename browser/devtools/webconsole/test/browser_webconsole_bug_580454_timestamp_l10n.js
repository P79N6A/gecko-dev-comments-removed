











"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  const TEST_TIMESTAMP = 12345678;
  let date = new Date(TEST_TIMESTAMP);
  let localizedString = WCUL10n.timestampString(TEST_TIMESTAMP);
  isnot(localizedString.indexOf(date.getHours()), -1, "the localized " +
        "timestamp contains the hours");
  isnot(localizedString.indexOf(date.getMinutes()), -1, "the localized " +
        "timestamp contains the minutes");
  isnot(localizedString.indexOf(date.getSeconds()), -1, "the localized " +
        "timestamp contains the seconds");
  isnot(localizedString.indexOf(date.getMilliseconds()), -1, "the localized " +
        "timestamp contains the milliseconds");
});
