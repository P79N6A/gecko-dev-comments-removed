

load(libdir + "asserts.js");

assertThrowsInstanceOf(
    () => Function("(a = function (obj) { with (obj) f(); }) => { 'use strict'; }"),
    SyntaxError);

assertThrowsInstanceOf(
    () => Function("(a = obj => { with (obj) f(); }) => { 'use strict'; }"),
    SyntaxError);
