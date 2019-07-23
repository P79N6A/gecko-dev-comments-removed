





































gTestfile = '15.5.4.11-3.js';






















var SECTION = "15.5.4.11-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.toLowerCase()";

writeHeaderToLog( SECTION + " "+ TITLE);



for ( var i = 0xFF00; i <= 0xFFEF; i++ ) {
  var U = new Unicode(i);






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
  u = GetUnicodeValues( c );
  this.upper = u[0];
  this.lower = u[1]
    return this;
}
function GetUnicodeValues( c ) {
  u = new Array();

  u[0] = c;
  u[1] = c;

  

  if ( c >= 0x0041 && c <= 0x005A) {
    u[0] = c;
    u[1] = c + 32;
    return u;
  }

  
  if ( c >= 0x0061 && c <= 0x007a ) {
    u[0] = c - 32;
    u[1] = c;
    return u;
  }

  
  if ( (c >= 0x00C0 && c <= 0x00D6) || (c >= 0x00D8 && c<=0x00DE) ) {
    u[0] = c;
    u[1] = c + 32;
    return u;
  }

  
  if ( (c >= 0x00E0 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FE) ) {
    u[0] = c - 32;
    u[1] = c;
    return u;
  }
  if ( c == 0x00FF ) {
    u[0] = 0x0178;
    u[1] = c;
    return u;
  }
  
  if ( (c >= 0x0100 && c < 0x0138) || (c > 0x0149 && c < 0x0178) ) {
    
    if ( c == 0x0130 ) {
      u[0] = c;
      u[1] = 0x0069;
      return u;
    }
    if ( c == 0x0131 ) {
      u[0] = 0x0049;
      u[1] = c;
      return u;
    }

    if ( c % 2 == 0 ) {
      
      u[0] = c;
      u[1] = c+1;
    } else {
      
      u[0] = c-1;
      u[1] = c;
    }
    return u;
  }
  if ( c == 0x0178 ) {
    u[0] = c;
    u[1] = 0x00FF;
    return u;
  }

  if ( (c >= 0x0139 && c < 0x0149) || (c > 0x0178 && c < 0x017F) ) {
    if ( c % 2 == 1 ) {
      
      u[0] = c;
      u[1] = c+1;
    } else {
      
      u[0] = c-1;
      u[1] = c;
    }
    return u;
  }
  if ( c == 0x017F ) {
    u[0] = 0x0053;
    u[1] = c;
  }

  
  

  if ( c >= 0x0200 && c <= 0x0217 ) {
    if ( c % 2 == 0 ) {
      u[0] = c;
      u[1] = c+1;
    } else {
      u[0] = c-1;
      u[1] = c;
    }
    return u;
  }

  
  
  

  
  

  
  

  
  
  

  
  

  if ( (c >= 0x0401 && c <= 0x040C) || ( c>= 0x040E && c <= 0x040F ) ) {
    u[0] = c;
    u[1] = c + 80;
    return u;
  }


  if ( c >= 0x0410  && c <= 0x042F ) {
    u[0] = c;
    u[1] = c + 32;
    return u;
  }

  if ( c >= 0x0430 && c<= 0x044F ) {
    u[0] = c - 32;
    u[1] = c;
    return u;

  }
  if ( (c >= 0x0451 && c <= 0x045C) || (c >=0x045E && c<= 0x045F) ) {
    u[0] = c -80;
    u[1] = c;
    return u;
  }

  if ( c >= 0x0460 && c <= 0x047F ) {
    if ( c % 2 == 0 ) {
      u[0] = c;
      u[1] = c +1;
    } else {
      u[0] = c - 1;
      u[1] = c;
    }
    return u;
  }

  
  
  if ( c >= 0x0531 && c <= 0x0556 ) {
    u[0] = c;
    u[1] = c + 48;
    return u;
  }
  if ( c >= 0x0561 && c < 0x0587 ) {
    u[0] = c - 48;
    u[1] = c;
    return u;
  }

  
  


  
  

  
  


  
  


  
  


  
  


  
  
  


  
  
  


  
  
  


  
  
  


  
  

  
  


  
  


  
  

  
  
  if ( c >= 0x10A0 && c <= 0x10C5 ) {
    u[0] = c;
    u[1] = c + 48;
    return u;
  }
  if ( c >= 0x10D0 && c <= 0x10F5 ) {
    u[0] = c;
    u[1] = c;
    return u;
  }

  
  

  
  
  


  
  

  
  

  
  


  
  
  


  
  
  


  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  


  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  
  

  if ( c >= 0xFF21 && c <= 0xFF3A ) {
    u[0] = c;
    u[1] = c + 32;
    return u;
  }

  if ( c >= 0xFF41 && c <= 0xFF5A ) {
    u[0] = c - 32;
    u[1] = c;
    return u;
  }

  
  

  return u;
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
