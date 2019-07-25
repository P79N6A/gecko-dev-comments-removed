

var g = newGlobal('new-compartment');
var dbg = new Debugger;
var gw = dbg.addDebuggee(g);

assertEq(gw.evalInGlobal("eval('\"Awake\"');").return, "Awake");



g.x = "Swing Lo Magellan";
g.y = "The Milk-Eyed Mender";
assertEq(gw.evalInGlobal("eval('var x = \"A Brief History of Love\"');\n"
                         + "var y = 'Merriweather Post Pavilion';"
                         + "x;").return,
         "A Brief History of Love");
assertEq(g.x, "A Brief History of Love");
assertEq(g.y, "Merriweather Post Pavilion");



g.x = "Swing Lo Magellan";
g.y = "The Milk-Eyed Mender";
assertEq(gw.evalInGlobalWithBindings("eval('var x = d1;'); var y = d2; x;",
                                     { d1: "A Brief History of Love",
                                       d2: "Merriweather Post Pavilion" }).return,
         "A Brief History of Love");
assertEq(g.x, "A Brief History of Love");
assertEq(g.y, "Merriweather Post Pavilion");







g.x = "Swing Lo Magellan";
g.y = "The Milk-Eyed Mender";
assertEq(gw.evalInGlobal("\'use strict\';\n"
                         + "eval('var x = \"A Brief History of Love\"');\n"
                         + "var y = \"Merriweather Post Pavilion\";"
                         + "x;").return,
         "Swing Lo Magellan");
assertEq(g.x, "Swing Lo Magellan");
assertEq(g.y, "The Milk-Eyed Mender");


g.x = "Swing Lo Magellan";
g.y = "The Milk-Eyed Mender";
assertEq(gw.evalInGlobalWithBindings("'use strict'; eval('var x = d1;'); var y = d2; x;",
                                     { d1: "A Brief History of Love",
                                       d2: "Merriweather Post Pavilion" }).return,
         "Swing Lo Magellan");
assertEq(g.x, "Swing Lo Magellan");
assertEq(g.y, "The Milk-Eyed Mender");
