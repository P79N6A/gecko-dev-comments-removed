



onmessage = function(event) {
  postMessage({event: "XMLHttpRequest",
               status: (new XMLHttpRequest() instanceof XMLHttpRequest),
               last: false });
  postMessage({event: "XMLHttpRequestUpload",
               status: ((new XMLHttpRequest()).upload instanceof XMLHttpRequestUpload),
               last: true });
}
