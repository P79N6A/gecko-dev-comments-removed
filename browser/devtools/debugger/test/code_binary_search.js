
(function() {

  window.binary_search = function(items, value) {
    var pivot, start, stop;
    start = 0;
    stop = items.length - 1;
    pivot = Math.floor((start + stop) / 2);
    while (items[pivot] !== value && start < stop) {
      if (value < items[pivot]) {
        stop = pivot - 1;
      }
      if (value > items[pivot]) {
        start = pivot + 1;
      }
      pivot = Math.floor((stop + start) / 2);
    }
    if (items[pivot] === value) {
      return pivot;
    } else {
      return -1;
    }
  };

}).call(this);




