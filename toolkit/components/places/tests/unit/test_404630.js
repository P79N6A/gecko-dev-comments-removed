





































function run_test() {
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
}
