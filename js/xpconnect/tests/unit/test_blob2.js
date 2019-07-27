



Components.utils.importGlobalProperties(['Blob', 'File']);

const Ci = Components.interfaces;

function run_test() {
  
  let testContent = "<a id=\"a\"><b id=\"b\">hey!<\/b><\/a>";
  
  var f1 = new Blob([testContent], {"type" : "text/xml"});

  
  do_check_true(f1 instanceof Ci.nsIDOMBlob, "Should be a DOM Blob");

  do_check_true(!(f1 instanceof File), "Should not be a DOM File");

  do_check_true(f1.type == "text/xml", "Wrong type");

  do_check_true(f1.size == testContent.length, "Wrong content size");

  var f2 = new Blob();
  do_check_true(f2.size == 0, "Wrong size");
  do_check_true(f2.type == "", "Wrong type");

  var threw = false;
  try {
    
    var f2 = new Blob(Date(132131532));
  } catch (e) {
    threw = true;
  }
  do_check_true(threw, "Passing a random object should fail");
}
