





































const Cc = Components.classes;
const Ci = Components.interfaces;


function run_test() {
  var result = Cc["@mozilla.org/autocomplete/controller;1"].
               createInstance(Ci.nsIAutoCompleteController);
  do_check_eq(result.searchStatus, Ci.nsIAutoCompleteController.STATUS_NONE);
}
