




































package org.mozilla.xpcom;


public class GREVersionRange {

  private String lower;
  private boolean lowerInclusive;
  private String upper;
  private boolean upperInclusive;

  public GREVersionRange(String aLower, boolean aLowerInclusive,
                         String aUpper, boolean aUpperInclusive) {
    lower = aLower;
    lowerInclusive = aLowerInclusive;
    upper = aUpper;
    upperInclusive = aUpperInclusive;
  }

  public boolean check(String aVersion) {
    VersionComparator comparator = new VersionComparator();
    int c = comparator.compare(aVersion, lower);
    if (c < 0) {
      return false;
    }

    if (c == 0 && !lowerInclusive) {
      return false;
    }

    c = comparator.compare(aVersion, upper);
    if (c > 0) {
      return false;
    }

    if (c == 0 && !upperInclusive) {
      return false;
    }

    return true;
  }

}

