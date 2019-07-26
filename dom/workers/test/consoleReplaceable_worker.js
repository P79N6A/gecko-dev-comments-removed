




onmessage = function(event) {
  postMessage({event: 'console exists', status: !!console, last : false});
  console = 42;
  postMessage({event: 'console is replaceable', status: console === 42, last : true});
}
