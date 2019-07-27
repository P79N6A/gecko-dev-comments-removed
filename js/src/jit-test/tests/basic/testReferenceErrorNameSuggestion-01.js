

function levenshteinDistance() {}

let e;
try {
  
  levenshtienDistance()
} catch (ee) {
  e = ee;
}

assertEq(e !== undefined, true);
assertEq(e.name, "ReferenceError");
assertEq(e.message.contains("did you mean 'levenshteinDistance'?"), true);
