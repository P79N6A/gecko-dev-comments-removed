





































function run_test() {
  do_test_pending();

  let exceptionCaught = false;
  try {
    PlacesUtils.favicons.setAndLoadFaviconForPage(
      null, uri("http://www.mozilla.com/favicon.ico"), false
    );
  } catch (ex) {
    exceptionCaught = true;
  }
  do_check_true(exceptionCaught, "should throw because page param is null");

  exceptionCaught = false;
  try {
    PlacesUtils.favicons.setAndLoadFaviconForPage(
      uri("http://www.mozilla.com"), null, false
    );
    do_throw("should throw because favicon param is null");
  } catch (ex) {
    exceptionCaught = true;
  }
  do_check_true(exceptionCaught, "should throw because page param is null");

  PlacesUtils.favicons.setAndLoadFaviconForPage(
    uri("http://www.google.com"), uri("http://www.google.com/favicon.ico"),
    false, continue_test
  );
}

function continue_test(aFaviconData) {
  do_test_finished();
}
