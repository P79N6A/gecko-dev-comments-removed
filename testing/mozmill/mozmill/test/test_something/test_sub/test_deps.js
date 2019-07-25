var jum = {}; Components.utils.import('resource://mozmill/modules/jum.js', jum);

var RELATIVE_ROOT = '..';

var MODULE_REQUIRES = ['test_parent'];

var setupModule = function(module) {
  
}

var testDependencies = function() {
  jum.assertEquals(test_parent.asdf, 'asdf');
}