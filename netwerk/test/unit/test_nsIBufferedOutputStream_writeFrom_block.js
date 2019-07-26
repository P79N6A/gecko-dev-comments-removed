function run_test() { run_next_test(); }

var CC = Components.Constructor;

var Pipe = CC('@mozilla.org/pipe;1', Ci.nsIPipe, 'init');
var BufferedOutputStream = CC('@mozilla.org/network/buffered-output-stream;1',
                              Ci.nsIBufferedOutputStream, 'init');
var ScriptableInputStream = CC('@mozilla.org/scriptableinputstream;1',
                               Ci.nsIScriptableInputStream, 'init');



add_test(function checkWouldBlockPipe() {
  
  var pipe = new Pipe(true, true, 1, 1);

  
  
  do_check_eq(pipe.outputStream.write('xy', 2), 1);
  do_check_eq(pipe.inputStream.available(), 1);

  do_check_throws_nsIException(() => pipe.outputStream.write('y', 1),
                               'NS_BASE_STREAM_WOULD_BLOCK');

  
  do_check_eq(pipe.inputStream.available(), 1);
  run_next_test();
});



add_test(function writeFromBlocksImmediately() {
  
  
  var outPipe = new Pipe(true, true, 1, 1);
  do_check_eq(outPipe.outputStream.write('x', 1), 1);

  
  
  var buffered = new BufferedOutputStream(outPipe.outputStream, 10);
  do_check_eq(buffered.write('0123456789', 10), 10);

  
  
  var inPipe = new Pipe(true, true, 1, 1);
  do_check_eq(inPipe.outputStream.write('y', 1), 1);

  do_check_eq(inPipe.inputStream.available(), 1);
  do_check_throws_nsIException(() => buffered.writeFrom(inPipe.inputStream, 1),
                               'NS_BASE_STREAM_WOULD_BLOCK');

  
  do_check_eq(inPipe.inputStream.available(), 1);

  run_next_test();
});




add_test(function writeFromReturnsPartialCountOnPartialFlush() {
  
  
  var outPipe = new Pipe(true, true, 5, 1);

  
  
  var outPipeReadable = new ScriptableInputStream(outPipe.inputStream);

  
  
  var buffered = new BufferedOutputStream(outPipe.outputStream, 7);

  
  var inPipe = new Pipe(true, true, 15, 1);

  
  do_check_eq(inPipe.outputStream.write('0123456789abcde', 15), 15);
  do_check_eq(inPipe.inputStream.available(), 15);

  
  
  
  
  do_check_eq(buffered.writeFrom(inPipe.inputStream, 11), 7);
  do_check_eq(outPipe.inputStream.available(), 5);
  do_check_eq(inPipe.inputStream.available(), 8);

  
  
  
  do_check_eq(buffered.writeFrom(inPipe.inputStream, 5), 5);
  do_check_eq(outPipe.inputStream.available(), 5);
  do_check_eq(inPipe.inputStream.available(), 3);

  
  do_check_throws_nsIException(() => buffered.writeFrom(inPipe.inputStream, 1),
                               'NS_BASE_STREAM_WOULD_BLOCK');

  
  do_check_eq(inPipe.inputStream.available(), 3);

  
  do_check_eq(outPipeReadable.available(), 5);
  do_check_eq(outPipeReadable.read(5), '01234');
  
  do_check_throws_nsIException(() => buffered.flush(), 'NS_ERROR_FAILURE');
  do_check_eq(outPipeReadable.available(), 5);
  do_check_eq(outPipeReadable.read(5), '56789');
  buffered.flush();
  do_check_eq(outPipeReadable.available(), 2);
  do_check_eq(outPipeReadable.read(2), 'ab');
  do_check_eq(buffered.writeFrom(inPipe.inputStream, 3), 3);
  buffered.flush();
  do_check_eq(outPipeReadable.available(), 3);
  do_check_eq(outPipeReadable.read(3), 'cde');

  run_next_test();
});



add_test(function writeFromReturnsPartialCountOnBlock() {
  
  
  var outPipe = new Pipe(true, true, 5, 1);

  
  
  var outPipeReadable = new ScriptableInputStream(outPipe.inputStream);

  
  
  var buffered = new BufferedOutputStream(outPipe.outputStream, 7);

  
  var inPipe = new Pipe(true, true, 15, 1);

  
  do_check_eq(inPipe.outputStream.write('0123456789abcde', 15), 15);
  do_check_eq(inPipe.inputStream.available(), 15);

  
  
  
  do_check_eq(buffered.writeFrom(inPipe.inputStream, 5), 5);
  buffered.flush();
  do_check_eq(outPipe.inputStream.available(), 5);
  do_check_eq(inPipe.inputStream.available(), 10);

  
  
  
  
  do_check_eq(buffered.writeFrom(inPipe.inputStream, 10), 7);
  do_check_eq(outPipe.inputStream.available(), 5);
  do_check_eq(inPipe.inputStream.available(), 3);

  
  do_check_throws_nsIException(() => buffered.writeFrom(inPipe.inputStream, 3),
                               'NS_BASE_STREAM_WOULD_BLOCK');

  
  do_check_eq(inPipe.inputStream.available(), 3);

  
  do_check_eq(outPipeReadable.available(), 5);
  do_check_eq(outPipeReadable.read(5), '01234');
  
  do_check_throws_nsIException(() => buffered.flush(), 'NS_ERROR_FAILURE');
  do_check_eq(outPipeReadable.available(), 5);
  do_check_eq(outPipeReadable.read(5), '56789');
  do_check_eq(buffered.writeFrom(inPipe.inputStream, 3), 3);
  buffered.flush();
  do_check_eq(outPipeReadable.available(), 5);
  do_check_eq(outPipeReadable.read(5), 'abcde');

  run_next_test();
});
