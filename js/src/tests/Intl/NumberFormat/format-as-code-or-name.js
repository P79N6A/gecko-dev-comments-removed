





var BUGNUMBER = 1093421;
var summary =
  "new Intl.NumberFormat(..., { style: 'currency', currency: '...', " +
  "currencyDisplay: 'name' or 'code' }) should have behavior other than " +
  "throwing";

print(BUGNUMBER + ": " + summary);





var usdCodeOptions =
  {
    style: "currency",
    currency: "USD",
    currencyDisplay: "code",
    minimumFractionDigits: 0,
    maximumFractionDigits: 0,
  };
var usDollarsCode = new Intl.NumberFormat("en-US", usdCodeOptions);
assertEq(/USD/.test(usDollarsCode.format(25)), true);







var xqqCodeOptions =
  {
    style: "currency",
    currency: "XQQ",
    currencyDisplay: "code",
    minimumFractionDigits: 0,
    maximumFractionDigits: 0,
  };
var xqqMoneyCode = new Intl.NumberFormat("en-US", xqqCodeOptions);
assertEq(/XQQ/.test(xqqMoneyCode.format(25)), true);




var usdNameOptions =
  {
    style: "currency",
    currency: "USD",
    currencyDisplay: "name",
    minimumFractionDigits: 0,
    maximumFractionDigits: 0,
  };
var usDollarsName = new Intl.NumberFormat("en-US", usdNameOptions);
assertEq(usDollarsName.format(25), "25 US dollars");



var xqqNameOptions =
  {
    style: "currency",
    currency: "XQQ",
    currencyDisplay: "name",
    minimumFractionDigits: 0,
    maximumFractionDigits: 0,
  };
var xqqMoneyName = new Intl.NumberFormat("en-US", xqqNameOptions);
assertEq(/XQQ/.test(xqqMoneyName.format(25)), true);

if (typeof reportCompare === "function")
  reportCompare(true, true);
