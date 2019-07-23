





































gTestsuite = 'lc3';



DT = Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass;

if ( typeof DT == "undefined" ) {
    throw "Test Exception:  "+
        "com.netscape.javascript.qa.liveconnect.DataTypeClass "+
        "is not on the CLASSPATH";
}







function TestCase( d, e, a ) {
  this.path = (typeof gTestPath == 'undefined') ?
    (gTestsuite + '/' + gTestsubsuite + '/' + gTestfile) :
    gTestPath;
  this.file = gTestfile;
  this.name        = d; 
  this.description = d;
  this.expect      = e;
  this.actual      = a;
  this.passed      = getTestCaseResult(e, a);
  this.reason      = "";
  this.bugnumber   = typeof(BUGNUMER) != 'undefined' ? BUGNUMBER : '';
  this.type = (typeof window == 'undefined' ? 'shell' : 'browser');
  gTestcases[gTc++] = this;
}
