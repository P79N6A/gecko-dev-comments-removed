 
 

 "use strict";



const TEST_URI = "data:text/html;charset=utf8,test for console output - 06";

let inputTests = [
  
  {
    input: 'Array(5)',
    output: 'Array [ <5 empty slots> ]',
    printOutput: ',,,,',
    inspectable: true,
    variablesViewLabel: "Array[5]",
  },
  
  {
    input: '[,1,2,3]',
    output: 'Array [ <1 empty slot>, 1, 2, 3 ]',
    printOutput: ",1,2,3",
    inspectable: true,
    variablesViewLabel: "Array[4]",
  },
  
  {
    input: '[,,,3,4,5]',
    output: 'Array [ <3 empty slots>, 3, 4, 5 ]',
    printOutput: ",,,3,4,5",
    inspectable: true,
    variablesViewLabel: "Array[6]",
  },
  
  {
    input: '[0,1,,3,4,5]',
    output: 'Array [ 0, 1, <1 empty slot>, 3, 4, 5 ]',
    printOutput: "0,1,,3,4,5",
    inspectable: true,
    variablesViewLabel: "Array[6]",
  },
  
  {
    input: '[0,1,,,,5]',
    output: 'Array [ 0, 1, <3 empty slots>, 5 ]',
    printOutput: "0,1,,,,5",
    inspectable: true,
    variablesViewLabel: "Array[6]",
  },
  
  {
    input: '[0,,2,,4,5]',
    output: 'Array [ 0, <1 empty slot>, 2, <1 empty slot>, 4, 5 ]',
    printOutput: "0,,2,,4,5",
    inspectable: true,
    variablesViewLabel: "Array[6]",
  },
  
  {
    input: '[0,,,3,,,,7,8]',
    output: 'Array [ 0, <2 empty slots>, 3, <3 empty slots>, 7, 8 ]',
    printOutput: "0,,,3,,,,7,8",
    inspectable: true,
    variablesViewLabel: "Array[9]",
  },
  
  {
    input: '[0,1,2,3,4,,]',
    output: 'Array [ 0, 1, 2, 3, 4, <1 empty slot> ]',
    printOutput: "0,1,2,3,4,",
    inspectable: true,
    variablesViewLabel: "Array[6]",
  },
  
  {
    input: '[0,1,2,,,,]',
    output: 'Array [ 0, 1, 2, <3 empty slots> ]',
    printOutput: "0,1,2,,,",
    inspectable: true,
    variablesViewLabel: "Array[6]",
  },

  
  {
    input: '[0,null,null,3,4,5]',
    output: 'Array [ 0, null, null, 3, 4, 5 ]',
    printOutput: "0,,,3,4,5",
    inspectable: true,
    variablesViewLabel: "Array[6]"
  },

  
  {
    input: '[0,undefined,undefined,3,4,5]',
    output: 'Array [ 0, undefined, undefined, 3, 4, 5 ]',
    printOutput: "0,,,3,4,5",
    inspectable: true,
    variablesViewLabel: "Array[6]"
  }
];

function test() {
  Task.spawn(function*() {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    return checkOutputForInputs(hud, inputTests);
  }).then(finishUp);
}

function finishUp() {
  inputTests = null;
  finishTest();
}
