




































package org.mozilla.xpcom;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Properties;
import java.util.StringTokenizer;





public class INIParser {

  private HashMap mSections;

  









  public INIParser(String aFilename, Charset aCharset)
          throws FileNotFoundException, IOException {
    initFromFile(new File(aFilename), aCharset);
  }

  







  public INIParser(String aFilename) throws FileNotFoundException, IOException {
    initFromFile(new File(aFilename), Charset.forName("UTF-8"));
  }

  








  public INIParser(File aFile, Charset aCharset)
          throws FileNotFoundException, IOException {
    initFromFile(aFile, aCharset);
  }

  







  public INIParser(File aFile) throws FileNotFoundException, IOException {
    initFromFile(aFile, Charset.forName("UTF-8"));
  }

  







  private void initFromFile(File aFile, Charset aCharset)
          throws FileNotFoundException, IOException {
    FileInputStream fileStream = new FileInputStream(aFile);
    InputStreamReader inStream = new InputStreamReader(fileStream, aCharset);
    BufferedReader reader = new BufferedReader(inStream);

    mSections = new HashMap();
    String currSection = null;

    String line;
    while ((line = reader.readLine()) != null) {
      
      String trimmedLine = line.trim();
      if (trimmedLine.length() == 0 || trimmedLine.startsWith("#")
              || trimmedLine.startsWith(";")) {
        continue;
      }

      
      if (line.startsWith("[")) {
        




        if (!trimmedLine.endsWith("]") ||
            trimmedLine.indexOf("]") != (trimmedLine.length() - 1)) {
          currSection = null;
          continue;
        }

        
        currSection = trimmedLine.substring(1, trimmedLine.length() - 1);
        continue;
      }

      
      if (currSection == null) {
        continue;
      }

      StringTokenizer tok = new StringTokenizer(line, "=");
      if (tok.countTokens() != 2) { 
        continue;
      }

      Properties props = (Properties) mSections.get(currSection);
      if (props == null) {
        props = new Properties();
        mSections.put(currSection, props);
      }
      props.setProperty(tok.nextToken(), tok.nextToken());
    }

    reader.close();
  }

  




  public Iterator getSections() {
    return mSections.keySet().iterator();
  }

  






  public Iterator getKeys(String aSection) {
    


    class PropertiesIterator implements Iterator {
      private Enumeration e;

      public PropertiesIterator(Enumeration aEnum) {
        e = aEnum;
      }

      public boolean hasNext() {
        return e.hasMoreElements();
      }

      public Object next() {
        return e.nextElement();
      }

      public void remove() {
        return;
      }
    }

    Properties props = (Properties) mSections.get(aSection);
    if (props == null) {
      return null;
    }

    return new PropertiesIterator(props.propertyNames());
  }

  






  public String getString(String aSection, String aKey) {
    Properties props = (Properties) mSections.get(aSection);
    if (props == null) {
      return null;
    }

    return props.getProperty(aKey);
  }

}

