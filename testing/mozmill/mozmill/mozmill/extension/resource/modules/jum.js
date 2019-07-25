




































var EXPORTED_SYMBOLS = ["assert", "assertTrue", "assertFalse", "assertEquals", "assertNotEquals",
                        "assertNull", "assertNotNull", "assertUndefined", "assertNotUndefined",
                        "assertNaN", "assertNotNaN", "fail", "pass"];

var frame = {}; Components.utils.import("resource://mozmill/modules/frame.js", frame);

var ifJSONable = function (v) {
  if (typeof(v) == 'function') {
    return undefined;
  } else {
    return v;
  }
}

var assert = function (booleanValue, comment) {
  if (booleanValue) {
    frame.events.pass({'function':'jum.assert', 'value':ifJSONable(booleanValue), 'comment':comment});
    return true;
  } else {
    frame.events.fail({'function':'jum.assert', 'value':ifJSONable(booleanValue), 'comment':comment});
    return false;
  }
}

var assertTrue = function (booleanValue, comment) {
  if (typeof(booleanValue) != 'boolean') {
    frame.events.fail({'function':'jum.assertTrue', 'value':ifJSONable(booleanValue),
                       'message':'Bad argument, value type '+typeof(booleanValue)+' !=  "boolean"', 
                       'comment':comment});
    return false;
  }
  
  if (booleanValue) {
    frame.events.pass({'function':'jum.assertTrue', 'value':ifJSONable(booleanValue), 
                       'comment':comment});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertTrue', 'value':ifJSONable(booleanValue), 
                       'comment':comment});
    return false;
  }
}

var assertFalse = function (booleanValue, comment) {
  if (typeof(booleanValue) != 'boolean') {
    frame.events.fail({'function':'jum.assertFalse', 'value':ifJSONable(booleanValue),
                       'message':'Bad argument, value type '+typeof(booleanValue)+' !=  "boolean"', 
                       'comment':comment});
    return false;
  }
  
  if (!booleanValue) {
    frame.events.pass({'function':'jum.assertFalse', 'value':ifJSONable(booleanValue), 
                       'comment':comment});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertFalse', 'value':ifJSONable(booleanValue), 
                       'comment':comment});
    return false;
  }
}

var assertEquals = function (value1, value2, comment) {
  if (value1 == value2) {
    frame.events.pass({'function':'jum.assertEquals', 'comment':comment,
                       'value1':ifJSONable(value1), 'value2':ifJSONable(value2)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertEquals', 'comment':comment,
                       'value1':ifJSONable(value1), 'value2':ifJSONable(value2)});
    return false;
  }
}

var assertNotEquals = function (value1, value2, comment) {
  if (value1 != value2) {
    frame.events.pass({'function':'jum.assertNotEquals', 'comment':comment,
                       'value1':ifJSONable(value1), 'value2':ifJSONable(value2)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNotEquals', 'comment':comment,
                       'value1':ifJSONable(value1), 'value2':ifJSONable(value2)});
    return false;
  }
}

var assertNull = function (value, comment) {
  if (value == null) {
    frame.events.pass({'function':'jum.assertNull', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNull', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var assertNull = function (value, comment) {
  if (value == null) {
    frame.events.pass({'function':'jum.assertNull', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNull', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var assertNotNull = function (value, comment) {
  if (value != null) {
    frame.events.pass({'function':'jum.assertNotNull', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNotNull', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var assertUndefined = function (value, comment) {
  if (value == undefined) {
    frame.events.pass({'function':'jum.assertUndefined', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertUndefined', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var assertNotUndefined = function (value, comment) {
  if (value != undefined) {
    frame.events.pass({'function':'jum.assertNotUndefined', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNotUndefined', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var assertNaN = function (value, comment) {
  if (isNaN(value)) {
    frame.events.pass({'function':'jum.assertNaN', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNaN', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var assertNotNaN = function (value, comment) {
  if (!isNaN(value)) {
    frame.events.pass({'function':'jum.assertNotNaN', 'comment':comment,
                       'value':ifJSONable(value)});
    return true;
  } else {
    frame.events.fail({'function':'jum.assertNotNaN', 'comment':comment,
                       'value':ifJSONable(value)});
    return false;
  }
}

var fail = function (comment) {
  frame.events.fail({'function':'jum.fail', 'comment':comment});
  return false;
}

var pass = function (comment) {
  frame.events.pass({'function':'jum.pass', 'comment':comment});
  return true;
}


