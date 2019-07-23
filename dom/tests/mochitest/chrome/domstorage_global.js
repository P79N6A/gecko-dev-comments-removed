function test_DOMStorage_global()
{
  var currentDomain = "mozilla.com";

  var globalStorage = window.globalStorage;
  is(globalStorage instanceof StorageList, true, "globalStorage property");

  var storage = globalStorage.namedItem(currentDomain);
  is(storage instanceof Storage, true, "StorageList namedItem");

  var storage2 = globalStorage[currentDomain];
  is(storage2 instanceof Storage, true, "StorageList property syntax");

  is(storage, storage2, "StorageList namedItem and array return same value");

  
  test_DOMStorage_global_Item(storage, "sample", null, 0, "initial");

  
  is(storage.getItem("", "Test"), null, "getItem no key");

  exh = false;
  try {
    storage.key(0);
  }
  catch (ex) { exh = true; }
  is(exh, true, "key index 0 too high");

  
  storage.setItem("sample", "This is the first message");
  var item = test_DOMStorage_global_Item(storage, "sample", "This is the first message", 1, "setItem");

  
  storage.test = "Second message";
  test_DOMStorage_global_Item(storage, "test", "Second message", 2, "setItem property");

  
  storage.sample = "Third message is this";
  var item2 = test_DOMStorage_global_Item(storage, "sample", "Third message is this", 2, "setItem property again");

  is(item.value, "Third message is this", "other item reference holds updated value");

  
  storage.setItem("test", "Look at this, the fourth message");
  test_DOMStorage_global_Item(storage, "test", "Look at this, the fourth message", 2, "setItem again");

  
  
  
  

  
  storage.setItem("", "Message Number Five");
  test_DOMStorage_global_Item(storage, "test", "Look at this, the fourth message", 2, "setItem no key");
  is(storage.getItem("", "Test"), null, "getItem no key");

  
  var key1 = storage.key(0);
  var key2 = storage.key(1);
  is(key1 != key2 && (key1 == "sample" || key1 == "test") &&
                                (key2 == "sample" || key2 == "test"), true, "key");
  exh = false;
  try {
    storage.key(2);
  }
  catch (ex) { exh = true; }
  is(exh, true, "key index 2 too high");

  storage.removeItem("sample");
  test_DOMStorage_global_Item(storage, "sample", null, 1, "removeItem");

  is(item.value, "", "other item reference holds deleted value");

  delete storage.test;
  test_DOMStorage_global_Item(storage, "test", null, 0, "delete item");

  storage.removeItem("test");

  

  
  
  var stringbit = "---abcdefghijklmnopqrstuvwxyz---";
  var longstring = "This is quite a long string:";
  for (var l = 0; l < 32767; l++)
    longstring += stringbit;

  var s;
  for (s = 0; s < 5; s++)
    storage.setItem("key" + s, longstring);

  
  exh = false;
  try {
    storage.setItem("keyfail", "One");
  }
  catch (ex) { exh = true; }
  is(exh, true, "per-domain size constraint");

  
  storage.setItem("key1", "This");

  
  exh = false;
  try {
    storage.setItem("keyfail", longstring);
  }
  catch (ex) { exh = true; }
  is(exh, true, "per-domain size constraint second check");

  
  storage.removeItem("key2");
  storage.setItem("key2", longstring);

  
  exh = false;
  try {
    storage.setItem(keyfail, longstring);
  }
  catch (ex) { exh = true; }
  is(exh, true, "per-domain size constraint third check");

  
  storage.setItem("key2", "Simple string");

  for (s = 0; s < 5; s++)
    storage.removeItem("key" + s);
}

function test_DOMStorage_global_Item(storage, key, expectedvalue, expectedlength, testid)
{
  var item = storage.getItem(key);
  if (expectedvalue != null)
    is(item instanceof StorageItem, true, testid + " is a Storage");

  is(expectedvalue == null ? item : "" + item, expectedvalue, testid + " getItem");

  var item2 = storage[key];
  
  if (testid != "removeItem") {
    if (item === null)
      is(item2, undefined, testid + " get property syntax");
    else
      is(item2, item, testid + " get property syntax");
  }
  if (expectedvalue != null)
    is(item2 instanceof StorageItem, true, testid + " property syntax is a Storage");

  if (expectedvalue != null)
    is(item.value, expectedvalue, testid + " value");

  is(storage.length, expectedlength, testid + " length");

  return item;
}

function is(left, right, str)
{
  window.opener.wrappedJSObject.SimpleTest.is(left, right, str);
}