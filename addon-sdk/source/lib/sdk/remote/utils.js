


"use strict";

const { Class } = require('../core/heritage');
const { List, addListItem, removeListItem } = require('../util/list');
const { emit } = require('../event/core');
const { pipe } = require('../event/utils');




const EventParent = Class({
  implements: [ List ],

  attachItem: function(item) {
    addListItem(this, item);

    pipe(item.port, this.port);
    pipe(item, this);

    item.once('detach', () => {
      removeListItem(this, item);
    })

    emit(this, 'attach', item);
  },

  
  
  forEvery: function(listener) {
    for (let item of this)
      listener(item);

    this.on('attach', listener);
  }
});
exports.EventParent = EventParent;
