



MARIONETTE_TIMEOUT = 1000;



setTimeout(function() { 
    is(1, 2); 
    finish();
}, 100);
isnot(1, 1);
ok(1 == 2);


