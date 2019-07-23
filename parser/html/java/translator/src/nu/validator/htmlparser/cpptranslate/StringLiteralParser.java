




































package nu.validator.htmlparser.cpptranslate;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class StringLiteralParser {

    private static final Pattern STRING_DECL = Pattern.compile("^.*\\(([^ ]+) = new nsString\\(\\)\\)->Assign\\(NS_LITERAL_STRING\\(\"([^\"]*)\"\\)\\);.*$");
    
    private final BufferedReader reader;
    
    public StringLiteralParser(Reader reader) {
        this.reader = new BufferedReader(reader);
    }
    
    public Map<String, String> parse() throws IOException {
        Map<String, String> map = new HashMap<String, String>();
        String line;
        while((line = reader.readLine()) != null) {
            Matcher m = STRING_DECL.matcher(line);
            if (m.matches()) {
                map.put(m.group(2), m.group(1));
            }
        }
        return map;
    }
    
}
