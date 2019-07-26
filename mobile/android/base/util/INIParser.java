



package org.mozilla.gecko.util;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Hashtable;

public final class INIParser extends INISection {
    
    private File mFile = null;

    
    private Hashtable<String, INISection> mSections = null;

    
    
    public INIParser(File iniFile) {
        super("");
        mFile = iniFile;
    }

    
    public void write() {
        writeTo(mFile);
    }

    
    public void writeTo(File f) {
        if (f == null)
            return;
  
        FileWriter outputStream = null;
        try {
            outputStream = new FileWriter(f);
        } catch (IOException e1) {
            e1.printStackTrace();
        }
  
        BufferedWriter writer = new BufferedWriter(outputStream);
        try {
            write(writer);
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void write(BufferedWriter writer) throws IOException {
        super.write(writer);

        if (mSections != null) {
            for (Enumeration<INISection> e = mSections.elements(); e.hasMoreElements();) {
                INISection section = e.nextElement();
                section.write(writer);
                writer.newLine();
            }
        }
    }

    
    public Hashtable<String, INISection> getSections() {
        if (mSections == null) {
            try {
                parse();
            } catch (IOException e) {
                debug("Error parsing: " + e);
            }
        }
        return mSections;
    }

    
    @Override
    protected void parse() throws IOException {
        super.parse();
        parse(mFile);
    }
   
    
    private void parse(File f) throws IOException {
        
        mSections = new Hashtable<String, INISection>();
  
        if (f == null || !f.exists())
            return;
  
        FileReader inputStream = null;
        try {
            inputStream = new FileReader(f);
        } catch (FileNotFoundException e1) {
            
            return;
        }
  
        BufferedReader buf = new BufferedReader(inputStream);
        String line = null;            
        INISection currentSection = null; 
  
        while ((line = buf.readLine()) != null) {
  
            if (line != null)
                line = line.trim();
  
            
            if (line == null || line.length() == 0 || line.charAt(0) == ';') {
                debug("Ignore line: " + line);
            } else if (line.charAt(0) == '[') {
                debug("Parse as section: " + line);
                currentSection = new INISection(line.substring(1, line.length()-1));
                mSections.put(currentSection.getName(), currentSection);
            } else {
                debug("Parse as property: " + line);
  
                String[] pieces = line.split("=");
                if (pieces.length != 2)
                    continue;
  
                String key = pieces[0].trim();
                String value = pieces[1].trim();
                if (currentSection != null) {
                    currentSection.setProperty(key, value);
                } else {
                    mProperties.put(key, value);
                }
            }
        }
        buf.close();
    }

    
    public void addSection(INISection sect) {
        
        getSections();
        mSections.put(sect.getName(), sect);
    }

    
    public INISection getSection(String key) {
        
        getSections();
        return mSections.get(key);
    }
 
    
    public void removeSection(String name) {
        
        getSections();
        mSections.remove(name);
    }

    
    
    public void renameSection(String oldName, String newName) {
        
        getSections();

        mSections.remove(newName);
        INISection section = mSections.get(oldName);
        if (section == null)
            return;

        mSections.remove(oldName);
        mSections.put(newName, section);
    }
}
