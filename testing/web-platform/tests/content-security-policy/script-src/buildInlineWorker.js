(function ()
{
 var test = new async_test("test inline worker");
 var workerSource = document.getElementById('inlineWorker');

 var blob = new Blob([workerSource.textContent]);

 
 var url = window.URL.createObjectURL(blob);

 var worker = new Worker(url);

 worker.addEventListener('message', function(e) {
    test.step(function () {
        assert_not_equals(e.data, 'fail', 'inline script ran');
        test.done();
    })
 }, false);

 worker.postMessage('');
})();
