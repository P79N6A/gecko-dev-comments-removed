


'use strict';

module.metadata = {
  "stability": "experimental"
};



exports.iteratorSymbol = Symbol.iterator;




function forInIterator() {
    for (let item of this)
        yield item;
}

exports.forInIterator = forInIterator;
