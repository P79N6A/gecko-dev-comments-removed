


























package ch.boye.httpclientandroidlib.conn.ssl;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.security.cert.Certificate;
import java.security.cert.CertificateParsingException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.NoSuchElementException;

import javax.net.ssl.SSLException;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.util.InetAddressUtils;
import ch.boye.httpclientandroidlib.util.TextUtils;

import ch.boye.httpclientandroidlib.NameValuePair;








@Immutable
public abstract class AbstractVerifier implements X509HostnameVerifier {

    









    private final static String[] BAD_COUNTRY_2LDS =
          { "ac", "co", "com", "ed", "edu", "go", "gouv", "gov", "info",
            "lg", "ne", "net", "or", "org" };

    static {
        
        Arrays.sort(BAD_COUNTRY_2LDS);
    }

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    public AbstractVerifier() {
        super();
    }

    public final void verify(final String host, final SSLSocket ssl)
          throws IOException {
        if(host == null) {
            throw new NullPointerException("host to verify is null");
        }

        SSLSession session = ssl.getSession();
        if(session == null) {
            
            
            
            final InputStream in = ssl.getInputStream();
            in.available();
            















            
            
            session = ssl.getSession();
            if(session == null) {
                
                
                ssl.startHandshake();

                
                
                session = ssl.getSession();
            }
        }

        final Certificate[] certs = session.getPeerCertificates();
        final X509Certificate x509 = (X509Certificate) certs[0];
        verify(host, x509);
    }

    public final boolean verify(final String host, final SSLSession session) {
        try {
            final Certificate[] certs = session.getPeerCertificates();
            final X509Certificate x509 = (X509Certificate) certs[0];
            verify(host, x509);
            return true;
        }
        catch(final SSLException e) {
            return false;
        }
    }

    public final void verify(final String host, final X509Certificate cert)
          throws SSLException {
        final String[] cns = getCNs(cert);
        final String[] subjectAlts = getSubjectAlts(cert, host);
        verify(host, cns, subjectAlts);
    }

    public final void verify(final String host, final String[] cns,
                             final String[] subjectAlts,
                             final boolean strictWithSubDomains)
          throws SSLException {

        
        
        
        
        final LinkedList<String> names = new LinkedList<String>();
        if(cns != null && cns.length > 0 && cns[0] != null) {
            names.add(cns[0]);
        }
        if(subjectAlts != null) {
            for (final String subjectAlt : subjectAlts) {
                if (subjectAlt != null) {
                    names.add(subjectAlt);
                }
            }
        }

        if(names.isEmpty()) {
            final String msg = "Certificate for <" + host + "> doesn't contain CN or DNS subjectAlt";
            throw new SSLException(msg);
        }

        
        final StringBuilder buf = new StringBuilder();

        
        
        final String hostName = normaliseIPv6Address(host.trim().toLowerCase(Locale.ENGLISH));
        boolean match = false;
        for(final Iterator<String> it = names.iterator(); it.hasNext();) {
            
            String cn = it.next();
            cn = cn.toLowerCase(Locale.ENGLISH);
            
            buf.append(" <");
            buf.append(cn);
            buf.append('>');
            if(it.hasNext()) {
                buf.append(" OR");
            }

            
            
            
            final String parts[] = cn.split("\\.");
            final boolean doWildcard =
                    parts.length >= 3 && parts[0].endsWith("*") &&
                    validCountryWildcard(cn) && !isIPAddress(host);

            if(doWildcard) {
                final String firstpart = parts[0];
                if (firstpart.length() > 1) { 
                    final String prefix = firstpart.substring(0, firstpart.length() - 1); 
                    final String suffix = cn.substring(firstpart.length()); 
                    final String hostSuffix = hostName.substring(prefix.length()); 
                    match = hostName.startsWith(prefix) && hostSuffix.endsWith(suffix);
                } else {
                    match = hostName.endsWith(cn.substring(1));
                }
                if(match && strictWithSubDomains) {
                    
                    
                    match = countDots(hostName) == countDots(cn);
                }
            } else {
                match = hostName.equals(normaliseIPv6Address(cn));
            }
            if(match) {
                break;
            }
        }
        if(!match) {
            throw new SSLException("hostname in certificate didn't match: <" + host + "> !=" + buf);
        }
    }

    


    @Deprecated
    public static boolean acceptableCountryWildcard(final String cn) {
        final String parts[] = cn.split("\\.");
        if (parts.length != 3 || parts[2].length() != 2) {
            return true; 
        }
        return Arrays.binarySearch(BAD_COUNTRY_2LDS, parts[1]) < 0;
    }

    boolean validCountryWildcard(final String cn) {
        final String parts[] = cn.split("\\.");
        if (parts.length != 3 || parts[2].length() != 2) {
            return true; 
        }
        return Arrays.binarySearch(BAD_COUNTRY_2LDS, parts[1]) < 0;
    }

    public static String[] getCNs(final X509Certificate cert) {
        final String subjectPrincipal = cert.getSubjectX500Principal().toString();
        try {
            return extractCNs(subjectPrincipal);
        } catch (SSLException ex) {
            return null;
        }
    }

    static String[] extractCNs(final String subjectPrincipal) throws SSLException {
        if (subjectPrincipal == null) {
            return null;
        }
        final List<String> cns = new ArrayList<String>();
        final List<NameValuePair> nvps = DistinguishedNameParser.INSTANCE.parse(subjectPrincipal);
        for (int i = 0; i < nvps.size(); i++) {
            final NameValuePair nvp = nvps.get(i);
            final String attribName = nvp.getName();
            final String attribValue = nvp.getValue();
            if (TextUtils.isBlank(attribValue)) {
                throw new SSLException(subjectPrincipal + " is not a valid X500 distinguished name");
            }
            if (attribName.equalsIgnoreCase("cn")) {
                cns.add(attribValue);
            }
        }
        return cns.isEmpty() ? null : cns.toArray(new String[ cns.size() ]);
    }

    







    private static String[] getSubjectAlts(
            final X509Certificate cert, final String hostname) {
        final int subjectType;
        if (isIPAddress(hostname)) {
            subjectType = 7;
        } else {
            subjectType = 2;
        }

        final LinkedList<String> subjectAltList = new LinkedList<String>();
        Collection<List<?>> c = null;
        try {
            c = cert.getSubjectAlternativeNames();
        }
        catch(final CertificateParsingException cpe) {
        }
        if(c != null) {
            for (final List<?> aC : c) {
                final List<?> list = aC;
                final int type = ((Integer) list.get(0)).intValue();
                if (type == subjectType) {
                    final String s = (String) list.get(1);
                    subjectAltList.add(s);
                }
            }
        }
        if(!subjectAltList.isEmpty()) {
            final String[] subjectAlts = new String[subjectAltList.size()];
            subjectAltList.toArray(subjectAlts);
            return subjectAlts;
        } else {
            return null;
        }
    }

    













    public static String[] getDNSSubjectAlts(final X509Certificate cert) {
        return getSubjectAlts(cert, null);
    }

    




    public static int countDots(final String s) {
        int count = 0;
        for(int i = 0; i < s.length(); i++) {
            if(s.charAt(i) == '.') {
                count++;
            }
        }
        return count;
    }

    private static boolean isIPAddress(final String hostname) {
        return hostname != null &&
            (InetAddressUtils.isIPv4Address(hostname) ||
                    InetAddressUtils.isIPv6Address(hostname));
    }

    


    private String normaliseIPv6Address(final String hostname) {
        if (hostname == null || !InetAddressUtils.isIPv6Address(hostname)) {
            return hostname;
        }
        try {
            final InetAddress inetAddress = InetAddress.getByName(hostname);
            return inetAddress.getHostAddress();
        } catch (final UnknownHostException uhe) { 
            log.error("Unexpected error converting "+hostname, uhe);
            return hostname;
        }
    }
}
