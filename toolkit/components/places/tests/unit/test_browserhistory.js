








































try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function run_test() {
  var testURI = uri("http://mozilla.com");

  




  try {
    bhist.addPageWithDetails(testURI, "testURI", Date.now());
  } catch(ex) {
    do_throw("addPageWithDetails failed");
  }

  



  do_check_eq("http://mozilla.com/", bhist.lastPageVisited);

  



  do_check_eq(1, bhist.count);

  


  try {
    bhist.removePage(testURI);
  } catch(ex) {
    do_throw("removePage failed");
  }
  do_check_eq(0, bhist.count);
  do_check_eq("", bhist.lastPageVisited);

  





  bhist.addPageWithDetails(testURI, "testURI", Date.now());
  bhist.removePagesFromHost("mozilla.com", true);
  do_check_eq(0, bhist.count);

  
  bhist.addPageWithDetails(testURI, "testURI", Date.now());
  bhist.addPageWithDetails(uri("http://foobar.mozilla.com"), "testURI2", Date.now());
  bhist.removePagesFromHost("mozilla.com", false);
  do_check_eq(1, bhist.count);

  



  bhist.removeAllPages();
  do_check_eq(0, bhist.count);

  







  
  
  
  

  




  
  
}
