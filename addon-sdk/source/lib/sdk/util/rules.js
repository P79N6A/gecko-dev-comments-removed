



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Class } = require('../core/heritage');
const { MatchPattern } = require('./match-pattern');
const { on, off, emit } = require('../event/core');
const { method } = require('../lang/functional');
const objectUtil = require('./object');
const { EventTarget } = require('../event/target');
const { List, addListItem, removeListItem } = require('./list');


const Rules = Class({
  implements: [
    EventTarget,
    List
  ],
  add: function(...rules) [].concat(rules).forEach(function onAdd(rule) {
    addListItem(this, rule);
    emit(this, 'add', rule);
  }, this),
  remove: function(...rules) [].concat(rules).forEach(function onRemove(rule) {
    removeListItem(this, rule);
    emit(this, 'remove', rule);
  }, this),
  get: function(rule) {
    let found = false;
    for (let i in this) if (this[i] === rule) found = true;
    return found;
  },
  
  matchesAny: function(uri) !!filterMatches(this, uri).length,
  toString: function() '[object Rules]'
});
exports.Rules = Rules;

function filterMatches(instance, uri) {
  let matches = [];
  for (let i in instance) {
    if (new MatchPattern(instance[i]).test(uri)) matches.push(instance[i]);
  }
  return matches;
}
