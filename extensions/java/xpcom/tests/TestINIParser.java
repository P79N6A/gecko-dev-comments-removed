




































import java.io.*;
import java.util.*;

import org.mozilla.xpcom.INIParser;


public class TestINIParser {

  





  public static void main(String[] args) throws FileNotFoundException,
          IOException {
    if (args.length > 1) {
      System.err.println("TestINIParser [file]");
      return;
    }

    INIParser parser = null;
    File iniFile = null;
    File tempFile = null;

    try {
      if (args.length == 1) {
        iniFile = new File(args[0]);
        parser = new INIParser(iniFile);
      } else {
        tempFile = File.createTempFile("testiniparser", null);
        createSampleINIFile(tempFile);
        parser = new INIParser(tempFile);
      }

      printValidSections(parser);
    } finally {
      if (tempFile != null) {
        tempFile.delete();
      }
    }
  }

  private static void createSampleINIFile(File aFile) throws IOException {
    BufferedWriter out = new BufferedWriter(new FileWriter(aFile));
    out.write("; first comment\n");
    out.write("; second comment\n");
    out.newLine();
    out.write("[good section 1]\n");
    out.write("param1=value1\n");
    out.write("param2=value2\n");
    out.newLine();
    out.write("[invalid section] 1\n");
    out.write("blah=blippity-blah\n");
    out.newLine();
    out.write("[good section 2]       \n");
    out.write("param3=value3\n");
    out.write("param4=value4\n");
    out.newLine();
    out.write("param5=value5\n");
    out.newLine();
    out.write("invalid pair\n");
    out.write("this shouldn't appear\n");
    out.write("   ; another comment\n");

    out.close();
  }

  private static void printValidSections(INIParser aParser) {
    Iterator sectionsIter = aParser.getSections();
    while (sectionsIter.hasNext()) {
      String sectionName = (String) sectionsIter.next();
      System.out.println("[" + sectionName + "]");

      Iterator keysIter = aParser.getKeys(sectionName);
      while (keysIter.hasNext()) {
        String key = (String) keysIter.next();
        String value = aParser.getString(sectionName, key);
        System.out.println(key + " = " + value);
      }

      System.out.println();
    }
  }

}

