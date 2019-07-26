



package org.mozilla.gecko.sync.setup.activities;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;

import android.util.Patterns;

public class WebURLFinder {
  public final List<String> candidates;

  public WebURLFinder(String string) {
    if (string == null) {
      throw new IllegalArgumentException("string must not be null");
    }

    this.candidates = candidateWebURLs(string);
  }

  public WebURLFinder(List<String> strings) {
    if (strings == null) {
      throw new IllegalArgumentException("strings must not be null");
    }

    this.candidates = candidateWebURLs(strings);
  }

  









  public static boolean isWebURL(String string) {
    try {
      new URI(string);
    } catch (Exception e) {
      return false;
    }

    if (android.webkit.URLUtil.isFileUrl(string) ||
        android.webkit.URLUtil.isJavaScriptUrl(string)) {
      return false;
    }

    return true;
  }

  







  public String bestWebURL() {
    String firstWebURLWithScheme = firstWebURLWithScheme();
    if (firstWebURLWithScheme != null) {
      return firstWebURLWithScheme;
    }

    return firstWebURLWithoutScheme();
  }

  protected static List<String> candidateWebURLs(Collection<String> strings) {
    List<String> candidates = new ArrayList<String>();

    for (String string : strings) {
      if (string == null) {
        continue;
      }

      candidates.addAll(candidateWebURLs(string));
    }

    return candidates;
  }

  protected static List<String> candidateWebURLs(String string) {
    Matcher matcher = Patterns.WEB_URL.matcher(string);
    List<String> matches = new LinkedList<String>();

    while (matcher.find()) {
      
      if (!isWebURL(matcher.group())) {
        continue;
      }

      
      if (matcher.start() > 0 && (string.charAt(matcher.start() - 1) == '@')) {
        continue;
      }

      matches.add(matcher.group());
    }

    return matches;
  }

  protected String firstWebURLWithScheme() {
    for (String match : candidates) {
      try {
        if (new URI(match).getScheme() != null) {
          return match;
        }
      } catch (URISyntaxException e) {
        
      }
    }

    return null;
  }

  protected String firstWebURLWithoutScheme() {
    if (!candidates.isEmpty()) {
      return candidates.get(0);
    }

    return null;
  }
}
