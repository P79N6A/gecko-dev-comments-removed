








$INCLUDE("testIntl.js");

taintProperties(["weekday", "era", "year", "month", "day", "hour", "minute", "second", "inDST"]);

var format = new Intl.DateTimeFormat();
var time = format.format();

