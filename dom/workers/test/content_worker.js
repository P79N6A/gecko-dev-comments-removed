



var props = {
  'ctypes': 1,
  'OS': 1
};
for (var prop in props) {
  postMessage({ "prop": prop, "value": self[prop] });
}
postMessage({ "testfinished": 1 });
