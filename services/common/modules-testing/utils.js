



"use strict";

const EXPORTED_SYMBOLS = [
  "TestingUtils",
];

let TestingUtils = {
  


  deepCopy: function deepCopy(thing, noSort) {
    if (typeof(thing) != "object" || thing == null) {
      return thing;
    }

    if (Array.isArray(thing)) {
      let ret = [];
      for (let element of thing) {
        ret.push(this.deepCopy(element, noSort));
      }

      return ret;
    }

    let ret = {};
    let props = [p for (p in thing)];

    if (!noSort) {
      props = props.sort();
    }

    for (let prop of props) {
      ret[prop] = this.deepCopy(thing[prop], noSort);
    }

    return ret;
  },
};
