








$INCLUDE("testIntl.js");

taintDataProperty(Intl, "DateTimeFormat");
new Date().toLocaleString();
new Date().toLocaleDateString();
new Date().toLocaleTimeString();
