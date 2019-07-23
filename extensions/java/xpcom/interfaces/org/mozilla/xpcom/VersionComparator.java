




































package org.mozilla.xpcom;

import java.util.Enumeration;
import java.util.StringTokenizer;

import org.mozilla.interfaces.nsISupports;
import org.mozilla.interfaces.nsIVersionComparator;































public class VersionComparator implements nsIVersionComparator {

  public nsISupports queryInterface(String aIID) {
    return Mozilla.queryInterface(this, aIID);
  }

  







  public int compare(String A, String B) {
    int result;
    String a = A, b = B;

    do {
      VersionPart va = new VersionPart();
      VersionPart vb = new VersionPart();
      a = parseVersionPart(a, va);
      b = parseVersionPart(b, vb);

      result = compareVersionPart(va, vb);
      if (result != 0) {
        break;
      }
    } while (a != null || b != null);

    return result;
  }

  private class VersionPart {
    int     numA = 0;
    String  strB;
    int     numC = 0;
    String  extraD;
  }

  private static String parseVersionPart(String aVersion, VersionPart result) {
    if (aVersion == null || aVersion.length() == 0) {
      return aVersion;
    }

    StringTokenizer tok = new StringTokenizer(aVersion.trim(), ".");
    String part = tok.nextToken();

    if (part.equals("*")) {
      result.numA = Integer.MAX_VALUE;
      result.strB = "";
    } else {
      VersionPartTokenizer vertok = new VersionPartTokenizer(part);
      try {
        result.numA = Integer.parseInt(vertok.nextToken());
      } catch (NumberFormatException e) {
        
        result.numA = 0;
      }

      if (vertok.hasMoreElements()) {
        String str = vertok.nextToken();

        
        if (str.charAt(0) == '+') {
          result.numA++;
          result.strB = "pre";
        } else {
          
          result.strB = str;

          if (vertok.hasMoreTokens()) {
            try {
              result.numC = Integer.parseInt(vertok.nextToken());
            } catch (NumberFormatException e) {
              
              result.numC = 0;
            }
            if (vertok.hasMoreTokens()) {
              result.extraD = vertok.getRemainder();
            }
          }
        }
      }
    }

    if (tok.hasMoreTokens()) {
      
      return aVersion.substring(part.length() + 1);
    }
    return null;
  }

  private int compareVersionPart(VersionPart va, VersionPart vb) {
    int res = compareInt(va.numA, vb.numA);
    if (res != 0) {
      return res;
    }

    res = compareString(va.strB, vb.strB);
    if (res != 0) {
      return res;
    }

    res = compareInt(va.numC, vb.numC);
    if (res != 0) {
      return res;
    }

    return compareString(va.extraD, vb.extraD);
  }

  private int compareInt(int n1, int n2) {
    return n1 - n2;
  }

  private int compareString(String str1, String str2) {
    
    if (str1 == null) {
      return (str2 != null) ? 1 : 0;
    }

    if (str2 == null) {
      return -1;
    }

    return str1.compareTo(str2);
  }

}







class VersionPartTokenizer implements Enumeration {

  String part;

  public VersionPartTokenizer(String aPart) {
    part = aPart;
  }

  public boolean hasMoreElements() {
    return part.length() != 0;
  }

  public boolean hasMoreTokens() {
    return part.length() != 0;
  }

  public Object nextElement() {
    if (part.matches("[\\+\\-]?[0-9].*")) {
      
      int index = 0;
      if (part.charAt(0) == '+' || part.charAt(0) == '-') {
        index = 1;
      }

      while (index < part.length() && Character.isDigit(part.charAt(index))) {
        index++;
      }

      String numPart = part.substring(0, index);
      part = part.substring(index);
      return numPart;
    } else {
      
      int index = 0;
      while (index < part.length() && !Character.isDigit(part.charAt(index))) {
        index++;
      }

      String alphaPart = part.substring(0, index);
      part = part.substring(index);
      return alphaPart;
    }
  }

  public String nextToken() {
    return (String) nextElement();
  }

  






  public String getRemainder() {
    return part;
  }

}

