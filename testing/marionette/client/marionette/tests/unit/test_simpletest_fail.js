



MARIONETTE_TIMEOUT = 1000;



setTimeout(function() { 
    is(1, 2, "is(1,2) should fail", TEST_UNEXPECTED_FAIL, TEST_PASS); 
    finish();
}, 100);
isnot(1, 1, "isnot(1,1) should fail", TEST_UNEXPECTED_FAIL, TEST_PASS);
ok(1 == 2, "ok(1==2) should fail", TEST_UNEXPECTED_FAIL, TEST_PASS);


