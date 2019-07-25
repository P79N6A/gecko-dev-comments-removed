



Object.create(evalcx('')).__defineSetter__('toString', function(){});
reportCompare(0, 0, "ok");
