




gTestfile = 'regress-524264';
uneval(function () { do yield; while (0); });
reportCompare("no assertion failure", "no assertion failure", "bug 524264");
