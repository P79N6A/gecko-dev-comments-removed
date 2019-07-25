



var BUGNUMBER = 620130;
var summary =
  "Calls to eval with same code + varying strict mode of script containing " +
  "eval == fail";

print(BUGNUMBER + ": " + summary);





function t(code) { return eval(code); }

assertEq(t("'use strict'; try { eval('with (5) 17'); } catch (e) { 'threw'; }"),
         "threw");
assertEq(t("try { eval('with (5) 17'); } catch (e) { 'threw'; }"),
         17);
assertEq(t("'use strict'; try { eval('with (5) 17'); } catch (e) { 'threw'; }"),
         "threw");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
