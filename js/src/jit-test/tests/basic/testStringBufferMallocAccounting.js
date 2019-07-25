
str = "a";
for (var i = 0; i < 20; ++i)
    str = str + str;
str.indexOf('a');


makeFinalizeObserver();
assertEq(finalizeCount(), 0);

for (var i = 0; i < 50; ++i)
    str.replace(/(a)/, '$1');
assertEq(finalizeCount(), 1);
