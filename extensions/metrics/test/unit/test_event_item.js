





































function run_test () {
  for (var i = 0; i < tests.length && tests[i][0]; ++i) {
    if (!tests[i][0].call()) {
      do_throw(tests[i][1]);
    }
  }
}

var tests = [
  [ test_create, "createEventItem failed" ],
  [ test_properties, "properties set/get failed" ],
  [ test_append_count, "appendChild/childCount failed" ],
  [ test_childat, "childAt failed" ],
  [ test_indexof, "indexOf failed" ],
  [ test_insert, "insertChildAt failed" ],
  [ test_remove, "removeChildAt failed" ],
  [ test_replace, "replaceChildAt failed" ],
  [ test_clear, "clearChildren failed" ],
  [ null ]
];

function test_create() {
  var item = createEventItem(EventNS, "test");
  do_check_neq(item, null);

  do_check_eq(item.itemNamespace, EventNS);
  do_check_eq(item.itemName, "test");
  do_check_eq(item.properties, null);
  do_check_eq(item.childCount, 0);
  return true;
}

function test_properties() {
  var item = createEventItem(EventNS, "test");
  var properties = {
    month: "April",
    year: 2006,
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.nsIPropertyBag) ||
          iid.equals(Components.interfaces.nsISupports)) {
        return this;
      }
    }
  };
  item.properties = properties;

  
  

  var month = item.properties.getProperty("month");
  do_check_eq(typeof(month), "string");
  do_check_eq(month, "April");

  var year = item.properties.getProperty("year");
  do_check_eq(typeof(year), "number");
  do_check_eq(year, 2006);

  var day = item.properties.getProperty("day");
  do_check_eq(day, undefined);

  return true;
}

function test_append_count() {
  var item = createEventItem(EventNS, "test");
  var children = buildItemChildren(item);
  do_check_eq(item.childCount, children.length);

  
  try {
    item.appendChild(null);
    do_throw("appendChild(null) should fail");
  } catch (e) {}

  do_check_eq(item.childCount, children.length);
  return true;
}

function test_childat() {
  var item = createEventItem(EventNS, "test");
  var children = buildItemChildren(item);

  
  compareItemChildren(item, children);

  
  try {
    item.childAt(-1);
    do_throw("childAt(-1)");
  } catch (e) {}
  try {
    item.childAt(children.length);
    do_throw("childAt(children.length)");
  } catch (e) {}

  return true;
}

function test_indexof() {
  var item = createEventItem(EventNS, "test");
  var children = buildItemChildren(item);
  for (var i = 0; i < children.length; ++i) {
    do_check_eq(item.indexOf(children[i]), i);
  }

  do_check_eq(item.indexOf(createEventItem(EventNS, "nothere")), -1);
  do_check_eq(item.indexOf(null), -1);

  return true;
}

function test_insert() {
  var item = createEventItem(EventNS, "test");
  var children = buildItemChildren(item);
  var i;

  var newChild = createEventItem(EventNS, "newchild");
  item.insertChildAt(newChild, 1);
  children.splice(1, 0, newChild);
  compareItemChildren(item, children);

  
  newChild = createEventItem(EventNS, "newchild2");
  item.insertChildAt(newChild, item.childCount);
  children.push(newChild);
  compareItemChildren(item, children);

  
  try {
    item.insertChildAt(newChild, -1);
    do_throw("insertChildAt(-1)");
  } catch (e) {}
  compareItemChildren(item, children);

  try {
    item.insertChildAt(newChild, item.childCount + 1);
    do_throw("insertChildAt past end");
  } catch (e) {}
  compareItemChildren(item, children);

  try {
    item.insertChildAt(null, item.childCount);
    do_throw("insertChildAt(null) should fail");
  } catch (e) {}
  compareItemChildren(item, children);

  return true;
}

function test_remove() {
  var item = createEventItem(EventNS, "test");
  var children = buildItemChildren(item);

  item.removeChildAt(3);
  children.splice(3, 1);
  compareItemChildren(item, children);

  
  try {
    item.removeChildAt(-1);
    do_throw("removeChildAt(-1)");
  } catch (e) {}
  compareItemChildren(item, children);

  try {
    item.removeChildAt(item.childCount);
    do_throw("removeChildAt past end");
  } catch (e) {}
  compareItemChildren(item, children);

  return true;
}

function test_replace() {
  var item = createEventItem(EventNS, "test");
  var children = buildItemChildren(item);

  var newChild = createEventItem(EventNS, "newchild");
  item.replaceChildAt(newChild, 6);
  children[6] = newChild;
  compareItemChildren(item, children);

  
  try {
    item.replaceChildAt(newChild, -1);
    do_throw("replaceChildAt(-1)");
  } catch (e) {}
  compareItemChildren(item, children);

  try {
    item.replaceChildAt(newChild, item.childCount);
    do_throw("replaceChildAt past end");
  } catch (e) {}
  compareItemChildren(item, children);

  return true;
}

function test_clear() {
  var item = createEventItem(EventNS, "test");
  buildItemChildren(item);

  item.clearChildren();
  compareItemChildren(item, []);

  item.clearChildren();
  compareItemChildren(item, []);

  return true;
}
