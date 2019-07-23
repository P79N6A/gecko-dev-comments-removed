









































try {
  var gh = Cc["@mozilla.org/browser/global-history;2"].
           getService(Ci.nsIGlobalHistory2);
} catch(ex) {
  do_throw("Could not get the global history service\n");
} 

function add_uri_to_history(aURI) {
  var referrer = uri("about:blank");
  gh.addURI(aURI,
            false, 
            true, 
            referrer);
}


function run_test() {
  
  var uri1 = uri("http://mozilla.com");
  add_uri_to_history(uri1);
  do_check_true(gh.isVisited(uri1));
 
  
  var uri2 = uri("https://etrade.com");
  add_uri_to_history(uri2);
  do_check_true(gh.isVisited(uri2));

  
  var uri3 = uri("ftp://ftp.mozilla.org");
  add_uri_to_history(uri3);
  do_check_true(gh.isVisited(uri3));

  
  var uri4 = uri("http://foobarcheese.com");
  do_check_false(gh.isVisited(uri4));

  
  
  
  var urlsToIgnore = ["about:config", 
    "data:,Hello%2C%20World!",
    "imap://cyrus.andrew.cmu.edu/archive.imap",
    "news://news.mozilla.org/mozilla.dev.apps.firefox",
    "moz-anno:favicon:http://www.mozilla.org/2005/made-up-favicon/84-1321",
    "chrome://browser/content/browser.xul",
    "view-source:http://www.google.com/"];

  for each (var currentURL in urlsToIgnore) {
    try {
      var cantAddUri = uri(currentURL);
    }
    catch(e) {
      
      
      
      print("Exception thrown for '" + currentURL + "', ignored.");
    }
    if (cantAddUri) {
      add_uri_to_history(cantAddUri);
      do_check_false(gh.isVisited(cantAddUri));
    }
  }
}
