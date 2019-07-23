






































var bug = 278873;
var summary = 'Don\'t Crash';
var actual = 'Crash';
var expect = 'No Crash';

  printBugNumber (bug);
  printStatus (summary);
  
function SwitchTest( input) {
    switch ( input ) {
        default:   break;
        case A:    break;
    }
}

printStatus(SwitchTest + '');

actual = 'No Crash';

  reportCompare(expect, actual, summary);
