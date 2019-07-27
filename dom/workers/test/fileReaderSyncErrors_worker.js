


function is(actual, expected, message) {
  var rtnObj = new Object();
  rtnObj.actual = actual;
  rtnObj.expected = expected;
  rtnObj.message = message;
  postMessage(rtnObj);
}




function writeProperty(file, property) {
  try {
    var oldValue = file[property];
    file[property] = -1;
    is(false, true, "Should have thrown an exception setting a read only property.");
  } catch (ex) {
    is(true, true, "Should have thrown an exception setting a read only property.");
  }
}





function fileReaderJunkArgument(blob) {
  var fileReader = new FileReaderSync();

  try {
    fileReader.readAsBinaryString(blob);
    is(false, true, "Should have thrown an exception calling readAsBinaryString.");
  } catch(ex) {
    is(true, true, "Should have thrown an exception.");
  }

  try {
    fileReader.readAsDataURL(blob);
    is(false, true, "Should have thrown an exception calling readAsDataURL.");
  } catch(ex) {
    is(true, true, "Should have thrown an exception.");
  }

  try {
    fileReader.readAsArrayBuffer(blob);
    is(false, true, "Should have thrown an exception calling readAsArrayBuffer.");
  } catch(ex) {
    is(true, true, "Should have thrown an exception.");
  }

  try {
    fileReader.readAsText(blob);
    is(false, true, "Should have thrown an exception calling readAsText.");
  } catch(ex) {
    is(true, true, "Should have thrown an exception.");
  }
}

onmessage = function(event) {
  var file = event.data;

  
  writeProperty(file, "size");
  writeProperty(file, "type");
  writeProperty(file, "name");
  writeProperty(file, "mozFullPath");

  
  fileReaderJunkArgument(undefined);
  fileReaderJunkArgument(-1);
  fileReaderJunkArgument(1);
  fileReaderJunkArgument(new Object());
  fileReaderJunkArgument("hello");

    
  postMessage(undefined);
};
