








$INCLUDE("testIntl.js");

taintDataProperty(Object.prototype, "1");
new Intl.NumberFormat("und");
