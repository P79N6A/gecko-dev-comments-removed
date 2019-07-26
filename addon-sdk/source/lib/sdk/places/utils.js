



'use strict';

module.metadata = {
  'stability': 'unstable'
};

const { Class } = require('../core/heritage');
const { method } = require('../lang/functional');
const { defer, promised, all } = require('../core/promise');





let TreeNode = Class({
  initialize: function (value) {
    this.value = value;
    this.children = [];
  },
  add: function (values) {
    [].concat(values).forEach(value => {
      this.children.push(value instanceof TreeNode ? value : TreeNode(value));
    });
  },
  get length () {
    let count = 0;
    this.walk(() => count++);
    
    return --count;
  },
  get: method(get),
  walk: method(walk),
  toString: function () '[object TreeNode]'
});
exports.TreeNode = TreeNode;






function walk (curr, fn) {
  return promised(fn)(curr).then(val => {
    return all(curr.children.map(child => walk(child, fn)));
  });
} 






function get (node, value) {
  if (node.value === value) return node;
  for (let child of node.children) {
    let found = get(child, value);
    if (found) return found;
  }
  return null;
}
