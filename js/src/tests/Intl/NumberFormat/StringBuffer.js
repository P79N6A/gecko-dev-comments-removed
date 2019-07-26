









var format = new Intl.NumberFormat("it-IT", {minimumFractionDigits: 1});

assertEq(format.format(1123123123123123123123.1), "1.123.123.123.123.120.000.000,0");
assertEq(format.format(12123123123123123123123.1), "12.123.123.123.123.100.000.000,0");
assertEq(format.format(123123123123123123123123.1), "123.123.123.123.123.000.000.000,0");

reportCompare(0, 0, "ok");
