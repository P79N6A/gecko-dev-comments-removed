




































package nu.validator.htmlparser.cpptranslate;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class GkAtomParser {

    private static final Pattern ATOM = Pattern.compile("^GK_ATOM\\(([^,]+),\\s*\"([^\"]*)\"\\).*$");
    
    private final BufferedReader reader;
    
    public GkAtomParser(Reader reader) {
        this.reader = new BufferedReader(reader);
    }
    
    public Map<String, String> parse() throws IOException {
        Map<String, String> map = new HashMap<String, String>();
        String line;
        while((line = reader.readLine()) != null) {
            Matcher m = ATOM.matcher(line);
            if (m.matches()) {
                map.put(m.group(2), m.group(1));
            }
        }
        return map;
    }
    
}
