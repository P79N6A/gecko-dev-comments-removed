





const collection = require("sdk/util/collection");

exports.testAddRemove = function (test) {
  let coll = new collection.Collection();
  compare(test, coll, []);
  addRemove(test, coll, [], false);
};

exports.testAddRemoveBackingArray = function (test) {
  let items = ["foo"];
  let coll = new collection.Collection(items);
  compare(test, coll, items);
  addRemove(test, coll, items, true);

  items = ["foo", "bar"];
  coll = new collection.Collection(items);
  compare(test, coll, items);
  addRemove(test, coll, items, true);
};

exports.testProperty = function (test) {
  let obj = makeObjWithCollProp();
  compare(test, obj.coll, []);
  addRemove(test, obj.coll, [], false);

  
  let items = ["foo"];
  obj.coll = items[0];
  compare(test, obj.coll, items);
  addRemove(test, obj.coll, items, false);

  
  items = ["foo", "bar"];
  obj.coll = items;
  compare(test, obj.coll, items);
  addRemove(test, obj.coll, items, false);
};

exports.testPropertyBackingArray = function (test) {
  let items = ["foo"];
  let obj = makeObjWithCollProp(items);
  compare(test, obj.coll, items);
  addRemove(test, obj.coll, items, true);

  items = ["foo", "bar"];
  obj = makeObjWithCollProp(items);
  compare(test, obj.coll, items);
  addRemove(test, obj.coll, items, true);
};





function addRemove(test, coll, initialItems, isBacking) {
  let items = isBacking ? initialItems : initialItems.slice(0);
  let numInitialItems = items.length;

  
  let numInsertions = 5;
  for (let i = 0; i < numInsertions; i++) {
    compare(test, coll, items);
    coll.add(i);
    if (!isBacking)
      items.push(i);
  }
  compare(test, coll, items);

  
  for (let i = 0; i < numInsertions; i++)
    coll.add(i);
  compare(test, coll, items);

  
  
  for (let i = 0; i < numInsertions; i++) {
    let val = i % 2 ? i - 1 :
              i === numInsertions - 1 ? i : i + 1;
    coll.remove(val);
    if (!isBacking)
      items.splice(items.indexOf(val), 1);
    compare(test, coll, items);
  }
  test.assertEqual(coll.length, numInitialItems,
                   "All inserted items should be removed");

  
  for (let i = 0; i < numInsertions; i++)
    coll.remove(i);
  compare(test, coll, items);

  
  let newItems = [0, 1];
  coll.add(newItems);
  compare(test, coll, isBacking ? items : items.concat(newItems));
  coll.remove(newItems);
  compare(test, coll, items);
  test.assertEqual(coll.length, numInitialItems,
                   "All inserted items should be removed");
}


function compare(test, coll, array) {
  test.assertEqual(coll.length, array.length,
                   "Collection length should be correct");
  let numItems = 0;
  for (let item in coll) {
    test.assertEqual(item, array[numItems], "Items should be equal");
    numItems++;
  }
  test.assertEqual(numItems, array.length,
                   "Number of items in iteration should be correct");
}



function makeObjWithCollProp(backingArray) {
  let obj = {};
  collection.addCollectionProperty(obj, "coll", backingArray);
  return obj;
}
