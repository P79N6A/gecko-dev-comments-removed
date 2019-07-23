






































try {
  var faviconService = Cc["@mozilla.org/browser/favicon-service;1"].
                       getService(Ci.nsIFaviconService);
} catch(ex) {
  do_throw("Could not get favicon service\n");
} 


function run_test() {
  try {
    faviconService.setAndLoadFaviconForPage(null, uri("http://www.mozilla.com/favicon.ico"), false);
    do_throw("should throw because page param is null");
  } catch (ex) {}

  try {
    faviconService.setAndLoadFaviconForPage(uri("http://www.mozilla.com"), null, false);
    do_throw("should throw because favicon param is null");
  } catch (ex) {}

  faviconService.setAndLoadFaviconForPage(uri("http://www.google.com"), uri("http://www.google.com/favicon.ico"), false);
}
