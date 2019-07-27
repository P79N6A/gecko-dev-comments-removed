

function rza() {}
function gza() {}
function odb() {}
function methodMan() {}
function inspectahDeck() {}
function raekwonTheChef() {}
function ghostfaceKillah() {}
function uGod() {}
function mastaKillah() {}

let e;
try {
  
  methoMan();
} catch (ee) {
  e = ee;
}

assertEq(e !== undefined, true);
assertEq(e.name, "ReferenceError");
assertEq(e.message.contains("did you mean 'methodMan'?"), true);
