








$INCLUDE("testIntl.js");

taintDataProperty(Intl, "Collator");
"a".localeCompare("b");
