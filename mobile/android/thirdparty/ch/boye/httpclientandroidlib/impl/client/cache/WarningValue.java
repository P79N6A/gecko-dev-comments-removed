

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;






class WarningValue {

    private int offs;
    private int init_offs;
    private final String src;
    private int warnCode;
    private String warnAgent;
    private String warnText;
    private Date warnDate;

    WarningValue(final String s) {
        this(s, 0);
    }

    WarningValue(final String s, final int offs) {
        this.offs = this.init_offs = offs;
        this.src = s;
        consumeWarnValue();
    }

    







    public static WarningValue[] getWarningValues(final Header h) {
        final List<WarningValue> out = new ArrayList<WarningValue>();
        final String src = h.getValue();
        int offs = 0;
        while(offs < src.length()) {
            try {
                final WarningValue wv = new WarningValue(src, offs);
                out.add(wv);
                offs = wv.offs;
            } catch (final IllegalArgumentException e) {
                final int nextComma = src.indexOf(',', offs);
                if (nextComma == -1) {
                    break;
                }
                offs = nextComma + 1;
            }
        }
        final WarningValue[] wvs = {};
        return out.toArray(wvs);
    }

    



    protected void consumeLinearWhitespace() {
        while(offs < src.length()) {
            switch(src.charAt(offs)) {
            case '\r':
                if (offs+2 >= src.length()
                    || src.charAt(offs+1) != '\n'
                    || (src.charAt(offs+2) != ' '
                        && src.charAt(offs+2) != '\t')) {
                    return;
                }
                offs += 2;
                break;
            case ' ':
            case '\t':
                break;
            default:
                return;
            }
            offs++;
        }
    }

    


    private boolean isChar(final char c) {
        final int i = c;
        return (i >= 0 && i <= 127);
    }

    



    private boolean isControl(final char c) {
        final int i = c;
        return (i == 127 || (i >=0 && i <= 31));
    }

    





    private boolean isSeparator(final char c) {
        return (c == '(' || c == ')' || c == '<' || c == '>'
                || c == '@' || c == ',' || c == ';' || c == ':'
                || c == '\\' || c == '\"' || c == '/'
                || c == '[' || c == ']' || c == '?' || c == '='
                || c == '{' || c == '}' || c == ' ' || c == '\t');
    }

    


    protected void consumeToken() {
        if (!isTokenChar(src.charAt(offs))) {
            parseError();
        }
        while(offs < src.length()) {
            if (!isTokenChar(src.charAt(offs))) {
                break;
            }
            offs++;
        }
    }

    private boolean isTokenChar(final char c) {
        return (isChar(c) && !isControl(c) && !isSeparator(c));
    }

    private static final String TOPLABEL = "\\p{Alpha}([\\p{Alnum}-]*\\p{Alnum})?";
    private static final String DOMAINLABEL = "\\p{Alnum}([\\p{Alnum}-]*\\p{Alnum})?";
    private static final String HOSTNAME = "(" + DOMAINLABEL + "\\.)*" + TOPLABEL + "\\.?";
    private static final String IPV4ADDRESS = "\\d+\\.\\d+\\.\\d+\\.\\d+";
    private static final String HOST = "(" + HOSTNAME + ")|(" + IPV4ADDRESS + ")";
    private static final String PORT = "\\d*";
    private static final String HOSTPORT = "(" + HOST + ")(\\:" + PORT + ")?";
    private static final Pattern HOSTPORT_PATTERN = Pattern.compile(HOSTPORT);

    protected void consumeHostPort() {
        final Matcher m = HOSTPORT_PATTERN.matcher(src.substring(offs));
        if (!m.find()) {
            parseError();
        }
        if (m.start() != 0) {
            parseError();
        }
        offs += m.end();
    }


    



    protected void consumeWarnAgent() {
        final int curr_offs = offs;
        try {
            consumeHostPort();
            warnAgent = src.substring(curr_offs, offs);
            consumeCharacter(' ');
            return;
        } catch (final IllegalArgumentException e) {
            offs = curr_offs;
        }
        consumeToken();
        warnAgent = src.substring(curr_offs, offs);
        consumeCharacter(' ');
    }

    



    protected void consumeQuotedString() {
        if (src.charAt(offs) != '\"') {
            parseError();
        }
        offs++;
        boolean foundEnd = false;
        while(offs < src.length() && !foundEnd) {
            final char c = src.charAt(offs);
            if (offs + 1 < src.length() && c == '\\'
                && isChar(src.charAt(offs+1))) {
                offs += 2;    
            } else if (c == '\"') {
                foundEnd = true;
                offs++;
            } else if (c != '\"' && !isControl(c)) {
                offs++;
            } else {
                parseError();
            }
        }
        if (!foundEnd) {
            parseError();
        }
    }

    


    protected void consumeWarnText() {
        final int curr = offs;
        consumeQuotedString();
        warnText = src.substring(curr, offs);
    }

    private static final String MONTH = "Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec";
    private static final String WEEKDAY = "Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday";
    private static final String WKDAY = "Mon|Tue|Wed|Thu|Fri|Sat|Sun";
    private static final String TIME = "\\d{2}:\\d{2}:\\d{2}";
    private static final String DATE3 = "(" + MONTH + ") ( |\\d)\\d";
    private static final String DATE2 = "\\d{2}-(" + MONTH + ")-\\d{2}";
    private static final String DATE1 = "\\d{2} (" + MONTH + ") \\d{4}";
    private static final String ASCTIME_DATE = "(" + WKDAY + ") (" + DATE3 + ") (" + TIME + ") \\d{4}";
    private static final String RFC850_DATE = "(" + WEEKDAY + "), (" + DATE2 + ") (" + TIME + ") GMT";
    private static final String RFC1123_DATE = "(" + WKDAY + "), (" + DATE1 + ") (" + TIME + ") GMT";
    private static final String HTTP_DATE = "(" + RFC1123_DATE + ")|(" + RFC850_DATE + ")|(" + ASCTIME_DATE + ")";
    private static final String WARN_DATE = "\"(" + HTTP_DATE + ")\"";
    private static final Pattern WARN_DATE_PATTERN = Pattern.compile(WARN_DATE);

    


    protected void consumeWarnDate() {
        final int curr = offs;
        final Matcher m = WARN_DATE_PATTERN.matcher(src.substring(offs));
        if (!m.lookingAt()) {
            parseError();
        }
        offs += m.end();
        warnDate = DateUtils.parseDate(src.substring(curr+1,offs-1));
    }

    


    protected void consumeWarnValue() {
        consumeLinearWhitespace();
        consumeWarnCode();
        consumeWarnAgent();
        consumeWarnText();
        if (offs + 1 < src.length() && src.charAt(offs) == ' ' && src.charAt(offs+1) == '\"') {
            consumeCharacter(' ');
            consumeWarnDate();
        }
        consumeLinearWhitespace();
        if (offs != src.length()) {
            consumeCharacter(',');
        }
    }

    protected void consumeCharacter(final char c) {
        if (offs + 1 > src.length()
            || c != src.charAt(offs)) {
            parseError();
        }
        offs++;
    }

    


    protected void consumeWarnCode() {
        if (offs + 4 > src.length()
            || !Character.isDigit(src.charAt(offs))
            || !Character.isDigit(src.charAt(offs + 1))
            || !Character.isDigit(src.charAt(offs + 2))
            || src.charAt(offs + 3) != ' ') {
            parseError();
        }
        warnCode = Integer.parseInt(src.substring(offs,offs+3));
        offs += 4;
    }

    private void parseError() {
        final String s = src.substring(init_offs);
        throw new IllegalArgumentException("Bad warn code \"" + s + "\"");
    }

    


    public int getWarnCode() { return warnCode; }

    




    public String getWarnAgent() { return warnAgent; }

    










    public String getWarnText() { return warnText; }

    




    public Date getWarnDate() { return warnDate; }

    








    @Override
    public String toString() {
        if (warnDate != null) {
            return String.format("%d %s %s \"%s\"", warnCode,
                    warnAgent, warnText, DateUtils.formatDate(warnDate));
        } else {
            return String.format("%d %s %s", warnCode, warnAgent, warnText);
        }
    }

}
