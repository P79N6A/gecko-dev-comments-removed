





































const Cc = Components.classes;
const Ci = Components.interfaces;

var gResultListener = {
  _lastResult: null,
  _lastValue: "",
  _lastRemoveFromDb: false,

  onValueRemoved: function(aResult, aValue, aRemoveFromDb) {
    this._lastResult = aResult;
    this._lastValue = aValue;
    this._lastRemoveFromDb = aRemoveFromDb;
  }
};



function run_test() {
  var result = Cc["@mozilla.org/autocomplete/simple-result;1"].
               createInstance(Ci.nsIAutoCompleteSimpleResult);
  result.appendMatch("a", "");
  result.appendMatch("b", "");
  result.appendMatch("c", "");
  result.setListener(gResultListener);
  do_check_eq(result.matchCount, 3);
  result.removeValueAt(0, true);
  do_check_eq(result.matchCount, 2);
  do_check_eq(gResultListener._lastResult, result);
  do_check_eq(gResultListener._lastValue, "a");
  do_check_eq(gResultListener._lastRemoveFromDb, true);

  result.removeValueAt(0, false);
  do_check_eq(result.matchCount, 1);
  do_check_eq(gResultListener._lastValue, "b");
  do_check_eq(gResultListener._lastRemoveFromDb, false);

  
  result.setListener(null);
  result.removeValueAt(0, true); 
  do_check_eq(result.matchCount, 0);
  do_check_eq(gResultListener._lastValue, "b");
}
