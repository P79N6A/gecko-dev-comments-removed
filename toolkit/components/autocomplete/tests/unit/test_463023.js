





































const Cc = Components.classes;
const Ci = Components.interfaces;


function run_test() {
  var result = Cc["@mozilla.org/autocomplete/simple-result;1"].
               createInstance(Ci.nsIAutoCompleteSimpleResult);
  do_check_eq(result.searchStatus, Ci.nsIAutoCompleteSimpleResult.STATUS_NONE);
}
