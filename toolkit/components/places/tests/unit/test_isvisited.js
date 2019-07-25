









































try {
  var gh = Cc["@mozilla.org/browser/global-history;2"].
           getService(Ci.nsIGlobalHistory2);
} catch(ex) {
  do_throw("Could not get the global history service\n");
} 

function add_uri_to_history(aURI, aCheckForGuid) {
  var referrer = uri("about:blank");
  gh.addURI(aURI,
            false, 
            true, 
            referrer);
  if (aCheckForGuid === undefined) {
    do_check_guid_for_uri(aURI);
  }
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

  
  
  
  const URLS = [
    "about:config",
    "imap://cyrus.andrew.cmu.edu/archive.imap",
    "news://new.mozilla.org/mozilla.dev.apps.firefox",
    "mailbox:Inbox",
    "moz-anno:favicon:http://mozilla.org/made-up-favicon",
    "view-source:http://mozilla.org",
    "chrome://browser/content/browser.xul",
    "resource://gre-resources/hiddenWindow.html",
    "data:,Hello%2C%20World!",
    "wyciwyg:/0/http://mozilla.org",
    "javascript:alert('hello wolrd!');",
  ];
  URLS.forEach(function(currentURL) {
    try {
      var cantAddUri = uri(currentURL);
    }
    catch(e) {
      
      
      
      do_log_info("Could not construct URI for '" + currentURL + "'; ignoring");
    }
    if (cantAddUri) {
      add_uri_to_history(cantAddUri, false);
      do_check_false(gh.isVisited(cantAddUri));
    }
  });
}
