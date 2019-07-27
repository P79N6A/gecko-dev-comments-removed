









var weirdCases =
  [
   "x-u-foo",
   "en-x-u-foo",
   "en-a-bar-x-u-foo",
   "en-x-u-foo-a-bar",
   "en-a-bar-u-baz-x-u-foo",
  ];

for (var locale of weirdCases)
  Intl.NumberFormat(locale).format(5);

if (typeof reportCompare === "function")
  reportCompare(true, true);
