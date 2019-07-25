


assertEq(JSON.stringify(undefined), undefined);
assertEq(JSON.stringify(function(){}), undefined);
assertEq(JSON.stringify(<x><y></y></x>), undefined);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
