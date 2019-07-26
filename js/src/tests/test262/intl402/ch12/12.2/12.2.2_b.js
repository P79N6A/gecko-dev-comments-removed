








$INCLUDE("testIntl.js");

taintDataProperty(Object.prototype, "1");
new Intl.DateTimeFormat("und");
