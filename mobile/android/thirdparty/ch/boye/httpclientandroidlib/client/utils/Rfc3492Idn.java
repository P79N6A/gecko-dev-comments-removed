

























package ch.boye.httpclientandroidlib.client.utils;

import java.util.StringTokenizer;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class Rfc3492Idn implements Idn {
    private static final int base = 36;
    private static final int tmin = 1;
    private static final int tmax = 26;
    private static final int skew = 38;
    private static final int damp = 700;
    private static final int initial_bias = 72;
    private static final int initial_n = 128;
    private static final char delimiter = '-';
    private static final String ACE_PREFIX = "xn--";

    private int adapt(final int delta, final int numpoints, final boolean firsttime) {
        int d = delta;
        if (firsttime) {
            d = d / damp;
        } else {
            d = d / 2;
        }
        d = d + (d / numpoints);
        int k = 0;
        while (d > ((base - tmin) * tmax) / 2) {
          d = d / (base - tmin);
          k = k + base;
        }
        return k + (((base - tmin + 1) * d) / (d + skew));
    }

    private int digit(final char c) {
        if ((c >= 'A') && (c <= 'Z')) {
            return (c - 'A');
        }
        if ((c >= 'a') && (c <= 'z')) {
            return (c - 'a');
        }
        if ((c >= '0') && (c <= '9')) {
            return (c - '0') + 26;
        }
        throw new IllegalArgumentException("illegal digit: "+ c);
    }

    public String toUnicode(final String punycode) {
        final StringBuilder unicode = new StringBuilder(punycode.length());
        final StringTokenizer tok = new StringTokenizer(punycode, ".");
        while (tok.hasMoreTokens()) {
            String t = tok.nextToken();
            if (unicode.length() > 0) {
                unicode.append('.');
            }
            if (t.startsWith(ACE_PREFIX)) {
                t = decode(t.substring(4));
            }
            unicode.append(t);
        }
        return unicode.toString();
    }

    protected String decode(final String s) {
        String input = s;
        int n = initial_n;
        int i = 0;
        int bias = initial_bias;
        final StringBuilder output = new StringBuilder(input.length());
        final int lastdelim = input.lastIndexOf(delimiter);
        if (lastdelim != -1) {
            output.append(input.subSequence(0, lastdelim));
            input = input.substring(lastdelim + 1);
        }

        while (input.length() > 0) {
            final int oldi = i;
            int w = 1;
            for (int k = base;; k += base) {
                if (input.length() == 0) {
                    break;
                }
                final char c = input.charAt(0);
                input = input.substring(1);
                final int digit = digit(c);
                i = i + digit * w; 
                final int t;
                if (k <= bias + tmin) {
                    t = tmin;
                } else if (k >= bias + tmax) {
                    t = tmax;
                } else {
                    t = k - bias;
                }
                if (digit < t) {
                    break;
                }
                w = w * (base - t); 
            }
            bias = adapt(i - oldi, output.length() + 1, (oldi == 0));
            n = n + i / (output.length() + 1); 
            i = i % (output.length() + 1);
            
            output.insert(i, (char) n);
            i++;
        }
        return output.toString();
    }

}
