



onclose = function() {
  postMessage("closed");
};

setTimeout(function () {
  setTimeout(function () {
    throw new Error("I should never run!");
  }, 1000);
  close();
}, 1000);
