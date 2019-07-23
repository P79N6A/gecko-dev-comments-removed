





































gTestfile = '15.5.4.11-4.js';






















var SECTION = "15.5.4.11-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.toLowerCase()";

writeHeaderToLog( SECTION + " "+ TITLE);




for ( var i = 0x3040; i <= 0x309F; i++ ) {
  var U = new Unicode( i );






  new TestCase(   SECTION,
		  "var s = new String( String.fromCharCode("+i+") ); s.toLowerCase().charCodeAt(0)",
		  U.lower,
		  eval("var s = new String( String.fromCharCode(i) ); s.toLowerCase().charCodeAt(0)") );
}

test();

function MyObject( value ) {
  this.value = value;
  this.substring = String.prototype.substring;
  this.toString = new Function ( "return this.value+''" );
}
function Unicode( c ) {
  this.upper = c;
  this.lower = c;

  

  if ( c >= 0x0041 && c <= 0x005A) {
    this.upper = c;
    this.lower = c + 32;
    return this;
  }

  
  if ( c >= 0x0061 && c <= 0x007a ) {
    this.upper = c - 32;
    this.lower = c;
    return this;
  }

  
  if ( (c >= 0x00C0 && c <= 0x00D6) || (c >= 0x00D8 && c<=0x00DE) ) {
    this.upper = c;
    this.lower = c + 32;
    return this;
  }

  
  if ( (c >= 0x00E0 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FE) ) {
    this.upper = c - 32;
    this.lower = c;
    return this;
  }
  if ( c == 0x00FF ) {
    this.upper = 0x0178;
    this.lower = c;
    return this;
  }
  
  if ( (c >= 0x0100 && c < 0x0138) || (c > 0x0149 && c < 0x0178) ) {
    
    if ( c == 0x0130 ) {
      this.upper = c;
      this.lower = 0x0069;
      return this;
    }
    if ( c == 0x0131 ) {
      this.upper = 0x0049;
      this.lower = c;
      return this;
    }

    if ( c % 2 == 0 ) {
      
      this.upper = c;
      this.lower = c+1;
    } else {
      
      this.upper = c-1;
      this.lower = c;
    }
    return this;
  }
  if ( c == 0x0178 ) {
    this.upper = c;
    this.lower = 0x00FF;
    return this;
  }

  if ( (c >= 0x0139 && c < 0x0149) || (c > 0x0178 && c < 0x017F) ) {
    if ( c % 2 == 1 ) {
      
      this.upper = c;
      this.lower = c+1;
    } else {
      
      this.upper = c-1;
      this.lower = c;
    }
    return this;
  }
  if ( c == 0x017F ) {
    this.upper = 0x0053;
    this.lower = c;
  }

  
  

  if ( c >= 0x0200 && c <= 0x0217 ) {
    if ( c % 2 == 0 ) {
      this.upper = c;
      this.lower = c+1;
    } else {
      this.upper = c-1;
      this.lower = c;
    }
    return this;
  }

  
  
  

  
  

  
  

  
  
  

  
  

  if ( (c >= 0x0401 && c <= 0x040C) || ( c>= 0x040E && c <= 0x040F ) ) {
    this.upper = c;
    this.lower = c + 80;
    return this;
  }


  if ( c >= 0x0410  && c <= 0x042F ) {
    this.upper = c;
    this.lower = c + 32;
    return this;
  }

  if ( c >= 0x0430 && c<= 0x044F ) {
    this.upper = c - 32;
    this.lower = c;
    return this;

  }
  if ( (c >= 0x0451 && c <= 0x045C) || (c >=0x045E && c<= 0x045F) ) {
    this.upper = c -80;
    this.lower = c;
    return this;
  }

  if ( c >= 0x0460 && c <= 0x047F ) {
    if ( c % 2 == 0 ) {
      this.upper = c;
      this.lower = c +1;
    } else {
      this.upper = c - 1;
      this.lower = c;
    }
    return this;
  }

  
  
  if ( c >= 0x0531 && c <= 0x0556 ) {
    this.upper = c;
    this.lower = c + 48;
    return this;
  }
  if ( c >= 0x0561 && c < 0x0587 ) {
    this.upper = c - 48;
    this.lower = c;
    return this;
  }

  
  


  
  

  
  


  
  


  
  


  
  


  
  
  


  
  
  


  
  
  


  
  
  


  
  

  
  


  
  


  
  

  
  
  if ( c >= 0x10A0 && c <= 0x10C5 ) {
    this.upper = c;
    this.lower = c + 48;
    return this;
  }
  if ( c >= 0x10D0 && c <= 0x10F5 ) {
    this.upper = c;
    this.lower = c;
    return this;
  }

  
  

  
  
  


  
  

  
  

  
  


  
  
  


  
  
  


  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  


  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  if ( c >= 0xFF21 && c <= 0xFF3A ) {
    this.upper = c;
    this.lower = c + 32;
    return this;
  }

  if ( c >= 0xFF41 && c <= 0xFF5A ) {
    this.upper = c - 32;
    this.lower = c;
    return this;
  }

  
  

  return this;
}

function DecimalToHexString( n ) {
  n = Number( n );
  var h = "0x";

  for ( var i = 3; i >= 0; i-- ) {
    if ( n >= Math.pow(16, i) ){
      var t = Math.floor( n  / Math.pow(16, i));
      n -= t * Math.pow(16, i);
      if ( t >= 10 ) {
	if ( t == 10 ) {
	  h += "A";
	}
	if ( t == 11 ) {
	  h += "B";
	}
	if ( t == 12 ) {
	  h += "C";
	}
	if ( t == 13 ) {
	  h += "D";
	}
	if ( t == 14 ) {
	  h += "E";
	}
	if ( t == 15 ) {
	  h += "F";
	}
      } else {
	h += String( t );
      }
    } else {
      h += "0";
    }
  }

  return h;
}
