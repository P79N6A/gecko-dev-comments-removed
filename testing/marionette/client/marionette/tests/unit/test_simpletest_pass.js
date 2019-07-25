



MARIONETTE_TIMEOUT = 1000;

is(2, 2, "test for is()");
isnot(2, 3, "test for isnot()");
ok(2 == 2, "test for ok()");

is(window.location.pathname.slice(-10), "empty.html");

setTimeout(finish, 100);

