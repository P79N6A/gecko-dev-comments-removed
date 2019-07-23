




































package nu.validator.htmlparser.cpptranslate;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;

public class LicenseExtractor {

    private final Reader reader;
    
    public LicenseExtractor(File file) throws IOException {
        this.reader = new InputStreamReader(new FileInputStream(file), "utf-8");
    }
    
    public String extract() throws IOException {
        boolean prevWasAsterisk = false;
        StringBuilder sb = new StringBuilder();
        int c;
        while ((c = reader.read()) != -1) {
            sb.append((char)c);
            switch (c) {
                case '*':
                    prevWasAsterisk = true;
                    continue;
                case '/':
                    if (prevWasAsterisk) {
                        return sb.toString();                        
                    }
                default:
                    prevWasAsterisk = false;
                    continue;
            }
        }
        return "";
    }
}
