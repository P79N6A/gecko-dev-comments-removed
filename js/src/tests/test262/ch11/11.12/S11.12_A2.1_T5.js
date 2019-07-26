










var y = new Object();
if ((true ? y : z) !== y) {
  $ERROR('#1: var y = new Object(); (true ? y : z) === y');
}

