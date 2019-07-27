
window.addEventListener("message", e => {
  console.log("frame content", "message", e);
  if ("title" in e.data) {
    document.title = e.data.title;
  }
});


window.setInterval(() => {
  
  
  var date = Date.now();
  var array = [];
  var i = 0;
  while (Date.now() - date <= 100) {
    array[i%2] = i++;
  }
  console.log("Arbitrary value", window.location, array);
}, 300);
