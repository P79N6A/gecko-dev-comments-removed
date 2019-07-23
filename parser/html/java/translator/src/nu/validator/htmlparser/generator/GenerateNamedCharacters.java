





















package nu.validator.htmlparser.generator;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Map;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class GenerateNamedCharacters {

    private static final int LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
    
    private static final Pattern LINE_PATTERN = Pattern.compile("^\\s*<tr> <td> <code title=\"\">([^<]*)</code> </td> <td> U\\+(\\S*) </td> </tr>.*$");
    
    private static String toUString(int c) {
        String hexString = Integer.toHexString(c);
        switch (hexString.length()) {
            case 1:
                return "\\u000" + hexString;
            case 2:
                return "\\u00" + hexString;
            case 3:
                return "\\u0" + hexString;
            case 4:
                return "\\u" + hexString;
            default:
                throw new RuntimeException("Unreachable.");
        }
    }

    
    



    public static void main(String[] args) throws IOException {
        TreeMap<String, String> entities = new TreeMap<String, String>();
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in, "utf-8"));
        String line;
        while ((line = reader.readLine()) != null) {
            Matcher m = LINE_PATTERN.matcher(line);
            if (m.matches()) {
                entities.put(m.group(1), m.group(2));
            }
        }
        System.out.println("static final char[][] NAMES = {");
        for (Map.Entry<String, String> entity : entities.entrySet()) {
            String name = entity.getKey();
            System.out.print("\"");
            System.out.print(name);
            System.out.println("\".toCharArray(),");
        }
        System.out.println("};");

        System.out.println("static final @NoLength char[][] VALUES = {");
        for (Map.Entry<String, String> entity : entities.entrySet()) {
            String value = entity.getValue();
            int intVal = Integer.parseInt(value, 16);
            System.out.print("{");
            if (intVal == '\'') {
                System.out.print("\'\\\'\'");                
            } else if (intVal == '\n') {
                System.out.print("\'\\n\'");                
            } else if (intVal == '\\') {
                System.out.print("\'\\\\\'");                
            } else if (intVal <= 0xFFFF) {
                System.out.print("\'");                
                System.out.print(toUString(intVal));                                
                System.out.print("\'");                
            } else {
                int hi = (LEAD_OFFSET + (intVal >> 10));
                int lo = (0xDC00 + (intVal & 0x3FF));
                System.out.print("\'");                
                System.out.print(toUString(hi));                                
                System.out.print("\', \'");                
                System.out.print(toUString(lo));                                
                System.out.print("\'");                
            }
            System.out.println("},");
        }
        System.out.println("};");

    }

}
