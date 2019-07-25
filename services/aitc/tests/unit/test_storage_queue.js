


Cu.import("resource://services-aitc/storage.js");
Cu.import("resource://services-common/async.js");

let queue = null;

function run_test() {
  initTestLogging();
  queue = new AitcQueue("test", run_next_test);
}

add_test(function test_queue_create() {
  do_check_eq(queue._queue.length, 0);
  do_check_eq(queue._writeLock, false);
  run_next_test();
});

add_test(function test_queue_enqueue() {
  
  let testObj = {foo: "bar"};
  queue.enqueue(testObj, function(err, done) {
    do_check_eq(err, null);
    do_check_true(done);

    
    do_check_eq(queue.peek(), testObj);
    
    do_check_eq(queue.peek(), testObj);

    run_next_test();
  });
});

add_test(function test_queue_dequeue() {
  
  queue.dequeue(function(err, done) {
    do_check_eq(err, null);
    do_check_true(done);
    do_check_eq(queue.length, 0);
    try {
      queue.peek();
    } catch (e) {
      do_check_eq(e.toString(), "Error: Queue is empty");
      run_next_test();
    }
  });
});

add_test(function test_queue_multiaddremove() {
  
  let items = [{test:"object"}, "teststring", 42];

  
  let num = Math.floor(Math.random() * 100 + 1);
  let rem = Math.floor(Math.random() * num + 1);

  
  for (let i = 0; i < rem; i++) {
    let ins = items[Math.round(Math.random() * 2)];
    let cb = Async.makeSpinningCallback();
    queue.enqueue(ins, cb);
    do_check_true(cb.wait());
  }

  do_check_eq(queue.length, rem);

  
  let check = [];
  for (let i = 0; i < (num - rem); i++) {
    check.push(items[Math.round(Math.random() * 2)]);
    let cb = Async.makeSpinningCallback();
    queue.enqueue(check[check.length - 1], cb);
    do_check_true(cb.wait());
  }

  do_check_eq(queue.length, num);

  
  for (let i = 0; i < rem; i++) {
    let cb = Async.makeSpinningCallback();
    queue.dequeue(cb);
    do_check_true(cb.wait());
  }

  do_check_eq(queue.length, num - rem);

  
  do_check_eq(JSON.stringify(queue._queue), JSON.stringify(check));

  
  let queue2 = new AitcQueue("test", function(done) {
    do_check_true(done);
    do_check_eq(queue2.length, queue.length);
    do_check_eq(JSON.stringify(queue._queue), JSON.stringify(queue2._queue));
    run_next_test();
  });
});

add_test(function test_queue_writelock() {
  
  queue._writeLock = true;
  let len = queue.length;

  queue.enqueue("writeLock test", function(err, done) {
    do_check_eq(err.toString(), "Error: _putFile already in progress");
    do_check_eq(queue.length, len);

    queue.dequeue(function(err, done) {
      do_check_eq(err.toString(), "Error: _putFile already in progress");
      do_check_eq(queue.length, len);
      run_next_test();
    });
  });
});
