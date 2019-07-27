(function ()
{
 var workerSource = document.getElementById('inlineWorker');
 var blob = new Blob([workerSource.textContent]);

 
 var url = window.URL.createObjectURL(blob);

 try {
   var worker = new Worker(url);
 }
 catch (e) {
   done();
 }

 worker.addEventListener('message', function(e) {
   assert_unreached("script ran");
 }, false);

 worker.postMessage('');
})();
