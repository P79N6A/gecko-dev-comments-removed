

























package ch.boye.httpclientandroidlib.impl.cookie;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.Collection;

import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class PublicSuffixListParser {
    private static final int MAX_LINE_LEN = 256;
    private final PublicSuffixFilter filter;

    PublicSuffixListParser(final PublicSuffixFilter filter) {
        this.filter = filter;
    }

    







    public void parse(final Reader list) throws IOException {
        final Collection<String> rules = new ArrayList<String>();
        final Collection<String> exceptions = new ArrayList<String>();
        final BufferedReader r = new BufferedReader(list);
        final StringBuilder sb = new StringBuilder(256);
        boolean more = true;
        while (more) {
            more = readLine(r, sb);
            String line = sb.toString();
            if (line.length() == 0) {
                continue;
            }
            if (line.startsWith("//"))
             {
                continue; 
            }
            if (line.startsWith("."))
             {
                line = line.substring(1); 
            }
            
            final boolean isException = line.startsWith("!");
            if (isException) {
                line = line.substring(1);
            }

            if (isException) {
                exceptions.add(line);
            } else {
                rules.add(line);
            }
        }

        filter.setPublicSuffixes(rules);
        filter.setExceptions(exceptions);
    }

    






    private boolean readLine(final Reader r, final StringBuilder sb) throws IOException {
        sb.setLength(0);
        int b;
        boolean hitWhitespace = false;
        while ((b = r.read()) != -1) {
            final char c = (char) b;
            if (c == '\n') {
                break;
            }
            
            if (Character.isWhitespace(c)) {
                hitWhitespace = true;
            }
            if (!hitWhitespace) {
                sb.append(c);
            }
            if (sb.length() > MAX_LINE_LEN)
             {
                throw new IOException("Line too long"); 
            }
        }
        return (b != -1);
    }
}
