


'use strict';

let xhr = new XMLHttpRequest();
xhr.open("GET", "data:text/plain,ok", true);
xhr.onload = function () {
  postMessage(xhr.responseText);
};
xhr.send(null);
