



'use strict';

try {
  require('./not-found');
}
catch (e1) {
  exports.firstError = e1;
  
  try {
    require('./not-found');
  }
  catch (e2) {
    exports.secondError = e2;
  }
}

try {
  require('./file.json');
}
catch (e) {
  exports.invalidJSON1 = e;
  try {
    require('./file.json');
  }
  catch (e) {
    exports.invalidJSON2 = e;
  }
}
