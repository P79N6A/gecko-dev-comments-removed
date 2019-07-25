



onerror = function(message, filename, lineno) {
  throw new Error("2");
};

onmessage = function(event) {
  throw new Error("1");
};
