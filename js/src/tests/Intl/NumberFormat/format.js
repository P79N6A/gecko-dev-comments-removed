






var format;


format = new Intl.NumberFormat("en-us");
assertEq(format.format(0), "0");
assertEq(format.format(-1), "-1");
assertEq(format.format(123456789.123456789), "123,456,789.123");




format = new Intl.NumberFormat("en-us", {style: "currency", currency: "USD"});
assertEq(format.format(0), "$0.00");
assertEq(format.format(-1), "($1.00)");
assertEq(format.format(123456789.123456789), "$123,456,789.12");



format = new Intl.NumberFormat("ja-jp", {style: "currency", currency: "JPY"});
assertEq(format.format(0), "￥0");
assertEq(format.format(-1), "-￥1");
assertEq(format.format(123456789.123456789), "￥123,456,789");



format = new Intl.NumberFormat("ar-jo", {style: "currency", currency: "JOD"});
assertEq(format.format(0), "د.أ.‏ ٠٫٠٠٠");
assertEq(format.format(-1), "د.أ.‏ ١٫٠٠٠-");
assertEq(format.format(123456789.123456789), "د.أ.‏ ١٢٣٤٥٦٧٨٩٫١٢٣");


format = new Intl.NumberFormat("th-th-u-nu-thai",
                               {style: "percent",
                                minimumSignificantDigits: 2,
                                maximumSignificantDigits: 2});
assertEq(format.format(0), "๐.๐๐%");
assertEq(format.format(-0.01), "-๑.๐%");
assertEq(format.format(1.10), "๑๑๐%");

reportCompare(0, 0, 'ok');
