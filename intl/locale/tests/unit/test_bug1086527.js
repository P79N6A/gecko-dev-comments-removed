







Components.utils.import("resource://gre/modules/PluralForm.jsm");

delete PluralForm.numForms;
delete PluralForm.get;
[PluralForm.get, PluralForm.numForms] = PluralForm.makeGetter(9);

function run_test() {
  "use strict";

  do_check_eq(3, PluralForm.numForms());
  do_check_eq("one", PluralForm.get(5, 'one;many'));
}
