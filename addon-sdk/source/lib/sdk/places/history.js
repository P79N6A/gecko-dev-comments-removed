



"use strict";

module.metadata = {
  "stability": "unstable",
  "engines": {
    "Firefox": "*",
    "SeaMonkey": "*"
  }
};




require('./host/host-bookmarks');
require('./host/host-tags');
require('./host/host-query');

const { Cc, Ci } = require('chrome');
const { Class } = require('../core/heritage');
const { events, send } = require('../addon/events');
const { defer, reject, all } = require('../core/promise');
const { uuid } = require('../util/uuid');
const { flatten } = require('../util/array');
const { has, extend, merge, pick } = require('../util/object');
const { emit } = require('../event/core');
const { defer: async } = require('../lang/functional');
const { EventTarget } = require('../event/target');
const {
  urlQueryParser, createQuery, createQueryOptions
} = require('./utils');





const HISTORY_QUERY = 0;

let search = function query (queries, options) {
  queries = [].concat(queries);
  let emitter = EventTarget();
  let queryObjs = queries.map(createQuery.bind(null, HISTORY_QUERY));
  let optionsObj = createQueryOptions(HISTORY_QUERY, options);

  
  
  async(() => {
    send('sdk-places-query', {
      query: queryObjs,
      options: optionsObj
    }).then(results => {
      results.map(item => emit(emitter, 'data', item));
      emit(emitter, 'end', results);
    }, reason => {
      emit(emitter, 'error', reason);
      emit(emitter, 'end', []);
    });
  })();

  return emitter;
};
exports.search = search;
