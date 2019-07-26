









var n = {};
var m = n;



if (typeof m !== "object") {
  $ERROR('#1: var n = {}; var m = n; typeof m === "object". Actual: ' + (typeof m));
}



function populateAge(person){person.age = 50;}

populateAge(m);



if (n.age !== 50) {
  $ERROR('#2: var n = {}; var m = n; function populateAge(person){person.age = 50;} populateAge(m); n.age === 50. Actual: ' + (n.age));
}





