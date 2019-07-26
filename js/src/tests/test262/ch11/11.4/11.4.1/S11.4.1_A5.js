













"use strict";

var reNames = Object.getOwnPropertyNames(RegExp);
for (var i = 0, len = reNames.length; i < len; i++) {
  var reName = reNames[i];
  if (reName !== 'prototype') {
    var deleted = 'unassigned';
    try {
      deleted = delete RegExp[reName];
    } catch (err) {
      if (!(err instanceof TypeError)) {
        $ERROR('#1: strict delete threw a non-TypeError: ' + err);
      }
      
    }
    if (deleted === false) {
      $ERROR('#2: Strict delete returned false');
    }
  }
}

