


"use strict";

const collection = require("sdk/util/collection");

exports.testAddRemove = function (assert) {
  let coll = new collection.Collection();
  compare(assert, coll, []);
  addRemove(assert, coll, [], false);
};

exports.testAddRemoveBackingArray = function (assert) {
  let items = ["foo"];
  let coll = new collection.Collection(items);
  compare(assert, coll, items);
  addRemove(assert, coll, items, true);

  items = ["foo", "bar"];
  coll = new collection.Collection(items);
  compare(assert, coll, items);
  addRemove(assert, coll, items, true);
};

exports.testProperty = function (assert) {
  let obj = makeObjWithCollProp();
  compare(assert, obj.coll, []);
  addRemove(assert, obj.coll, [], false);

  
  let items = ["foo"];
  obj.coll = items[0];
  compare(assert, obj.coll, items);
  addRemove(assert, obj.coll, items, false);

  
  items = ["foo", "bar"];
  obj.coll = items;
  compare(assert, obj.coll, items);
  addRemove(assert, obj.coll, items, false);
};

exports.testPropertyBackingArray = function (assert) {
  let items = ["foo"];
  let obj = makeObjWithCollProp(items);
  compare(assert, obj.coll, items);
  addRemove(assert, obj.coll, items, true);

  items = ["foo", "bar"];
  obj = makeObjWithCollProp(items);
  compare(assert, obj.coll, items);
  addRemove(assert, obj.coll, items, true);
};





function addRemove(assert, coll, initialItems, isBacking) {
  let items = isBacking ? initialItems : initialItems.slice(0);
  let numInitialItems = items.length;

  
  let numInsertions = 5;
  for (let i = 0; i < numInsertions; i++) {
    compare(assert, coll, items);
    coll.add(i);
    if (!isBacking)
      items.push(i);
  }
  compare(assert, coll, items);

  
  for (let i = 0; i < numInsertions; i++)
    coll.add(i);
  compare(assert, coll, items);

  
  
  for (let i = 0; i < numInsertions; i++) {
    let val = i % 2 ? i - 1 :
              i === numInsertions - 1 ? i : i + 1;
    coll.remove(val);
    if (!isBacking)
      items.splice(items.indexOf(val), 1);
    compare(assert, coll, items);
  }
  assert.equal(coll.length, numInitialItems,
               "All inserted items should be removed");

  
  for (let i = 0; i < numInsertions; i++)
    coll.remove(i);
  compare(assert, coll, items);

  
  let newItems = [0, 1];
  coll.add(newItems);
  compare(assert, coll, isBacking ? items : items.concat(newItems));
  coll.remove(newItems);
  compare(assert, coll, items);
  assert.equal(coll.length, numInitialItems,
               "All inserted items should be removed");
}


function compare(assert, coll, array) {
  assert.equal(coll.length, array.length,
               "Collection length should be correct");
  let numItems = 0;
  for (let item in coll) {
    assert.equal(item, array[numItems], "Items should be equal");
    numItems++;
  }
  assert.equal(numItems, array.length,
               "Number of items in iteration should be correct");
}



function makeObjWithCollProp(backingArray) {
  let obj = {};
  collection.addCollectionProperty(obj, "coll", backingArray);
  return obj;
}

require("sdk/test").run(exports);
