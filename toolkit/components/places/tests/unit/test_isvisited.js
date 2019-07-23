









































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

  
  
  
  
  
  
  
  
}
