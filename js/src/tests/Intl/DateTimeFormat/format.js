







var format;
var date = Date.UTC(2012, 11, 12, 3, 0, 0);
var longFormatOptions = {timeZone: "UTC",
                         year: "numeric", month: "long", day: "numeric",
                         hour: "numeric", minute: "numeric", second: "numeric"};


format = new Intl.DateTimeFormat("en-us", {timeZone: "UTC"});
assertEq(format.format(date), "12/12/2012");



format = new Intl.DateTimeFormat("th-th", {timeZone: "UTC"});
assertEq(format.format(date), "12/12/2555");


format = new Intl.DateTimeFormat("th-th-u-nu-thai", longFormatOptions);
assertEq(format.format(date), "๑๒ ธันวาคม ๒๕๕๕, ๐๓:๐๐:๐๐");


format = new Intl.DateTimeFormat("ja-jp", longFormatOptions);
assertEq(format.format(date), "2012年12月12日 3:00:00");


format = new Intl.DateTimeFormat("ar-ma-u-ca-islamicc", longFormatOptions);
assertEq(format.format(date), "28 محرم، 1434 3:00:00 ص");

reportCompare(0, 0, 'ok');
