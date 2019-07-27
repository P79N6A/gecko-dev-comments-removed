


'use strict';

module.metadata = {
  "stability": "experimental"
};




const JS_HAS_SYMBOLS = typeof Symbol === "function";
exports.iteratorSymbol = JS_HAS_SYMBOLS ? Symbol.iterator : "@@iterator";




function forInIterator() {
    for (let item of this)
        yield item;
}

exports.forInIterator = forInIterator;
