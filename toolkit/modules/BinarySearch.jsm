



"use strict";

this.EXPORTED_SYMBOLS = [
  "BinarySearch",
];

this.BinarySearch = Object.freeze({

  







  indexOf: function (comparator, array, target) {
    let [found, idx] = this.search(comparator, array, target);
    return found ? idx : -1;
  },

  








  insertionIndexOf: function (comparator, array, target) {
    return this.search(comparator, array, target)[1];
  },

  



















  search: function (comparator, array, target) {
    let low = 0;
    let high = array.length - 1;
    while (low <= high) {
      
      let mid = (low + high) >> 1;
      let cmp = comparator(target, array[mid]);
      if (cmp == 0)
        return [true, mid];
      if (cmp < 0)
        high = mid - 1;
      else
        low = mid + 1;
    }
    return [false, low];
  },
});
