


load(libdir + 'asserts.js');
assertThrowsInstanceOf(function() {
  for each(z in [0, 0, 0, 0]) { ({
      __parent__: []
    } = [])
  }
}, TypeError); 
