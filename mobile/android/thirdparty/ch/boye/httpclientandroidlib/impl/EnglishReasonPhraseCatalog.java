


























package ch.boye.httpclientandroidlib.impl;

import java.util.Locale;

import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.ReasonPhraseCatalog;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;








@Immutable
public class EnglishReasonPhraseCatalog implements ReasonPhraseCatalog {

    

    




    public final static EnglishReasonPhraseCatalog INSTANCE =
        new EnglishReasonPhraseCatalog();


    



    protected EnglishReasonPhraseCatalog() {
        
    }


    







    public String getReason(final int status, final Locale loc) {
        Args.check(status >= 100 && status < 600, "Unknown category for status code " + status);
        final int category = status / 100;
        final int subcode  = status - 100*category;

        String reason = null;
        if (REASON_PHRASES[category].length > subcode) {
            reason = REASON_PHRASES[category][subcode];
        }

        return reason;
    }


    
    private static final String[][] REASON_PHRASES = new String[][]{
        null,
        new String[3],  
        new String[8],  
        new String[8],  
        new String[25], 
        new String[8]   
    };



    






    private static void setReason(final int status, final String reason) {
        final int category = status / 100;
        final int subcode  = status - 100*category;
        REASON_PHRASES[category][subcode] = reason;
    }


    

    
    static {
        
        setReason(HttpStatus.SC_OK,
                  "OK");
        setReason(HttpStatus.SC_CREATED,
                  "Created");
        setReason(HttpStatus.SC_ACCEPTED,
                  "Accepted");
        setReason(HttpStatus.SC_NO_CONTENT,
                  "No Content");
        setReason(HttpStatus.SC_MOVED_PERMANENTLY,
                  "Moved Permanently");
        setReason(HttpStatus.SC_MOVED_TEMPORARILY,
                  "Moved Temporarily");
        setReason(HttpStatus.SC_NOT_MODIFIED,
                  "Not Modified");
        setReason(HttpStatus.SC_BAD_REQUEST,
                  "Bad Request");
        setReason(HttpStatus.SC_UNAUTHORIZED,
                  "Unauthorized");
        setReason(HttpStatus.SC_FORBIDDEN,
                  "Forbidden");
        setReason(HttpStatus.SC_NOT_FOUND,
                  "Not Found");
        setReason(HttpStatus.SC_INTERNAL_SERVER_ERROR,
                  "Internal Server Error");
        setReason(HttpStatus.SC_NOT_IMPLEMENTED,
                  "Not Implemented");
        setReason(HttpStatus.SC_BAD_GATEWAY,
                  "Bad Gateway");
        setReason(HttpStatus.SC_SERVICE_UNAVAILABLE,
                  "Service Unavailable");

        
        setReason(HttpStatus.SC_CONTINUE,
                  "Continue");
        setReason(HttpStatus.SC_TEMPORARY_REDIRECT,
                  "Temporary Redirect");
        setReason(HttpStatus.SC_METHOD_NOT_ALLOWED,
                  "Method Not Allowed");
        setReason(HttpStatus.SC_CONFLICT,
                  "Conflict");
        setReason(HttpStatus.SC_PRECONDITION_FAILED,
                  "Precondition Failed");
        setReason(HttpStatus.SC_REQUEST_TOO_LONG,
                  "Request Too Long");
        setReason(HttpStatus.SC_REQUEST_URI_TOO_LONG,
                  "Request-URI Too Long");
        setReason(HttpStatus.SC_UNSUPPORTED_MEDIA_TYPE,
                  "Unsupported Media Type");
        setReason(HttpStatus.SC_MULTIPLE_CHOICES,
                  "Multiple Choices");
        setReason(HttpStatus.SC_SEE_OTHER,
                  "See Other");
        setReason(HttpStatus.SC_USE_PROXY,
                  "Use Proxy");
        setReason(HttpStatus.SC_PAYMENT_REQUIRED,
                  "Payment Required");
        setReason(HttpStatus.SC_NOT_ACCEPTABLE,
                  "Not Acceptable");
        setReason(HttpStatus.SC_PROXY_AUTHENTICATION_REQUIRED,
                  "Proxy Authentication Required");
        setReason(HttpStatus.SC_REQUEST_TIMEOUT,
                  "Request Timeout");

        setReason(HttpStatus.SC_SWITCHING_PROTOCOLS,
                  "Switching Protocols");
        setReason(HttpStatus.SC_NON_AUTHORITATIVE_INFORMATION,
                  "Non Authoritative Information");
        setReason(HttpStatus.SC_RESET_CONTENT,
                  "Reset Content");
        setReason(HttpStatus.SC_PARTIAL_CONTENT,
                  "Partial Content");
        setReason(HttpStatus.SC_GATEWAY_TIMEOUT,
                  "Gateway Timeout");
        setReason(HttpStatus.SC_HTTP_VERSION_NOT_SUPPORTED,
                  "Http Version Not Supported");
        setReason(HttpStatus.SC_GONE,
                  "Gone");
        setReason(HttpStatus.SC_LENGTH_REQUIRED,
                  "Length Required");
        setReason(HttpStatus.SC_REQUESTED_RANGE_NOT_SATISFIABLE,
                  "Requested Range Not Satisfiable");
        setReason(HttpStatus.SC_EXPECTATION_FAILED,
                  "Expectation Failed");

        
        setReason(HttpStatus.SC_PROCESSING,
                  "Processing");
        setReason(HttpStatus.SC_MULTI_STATUS,
                  "Multi-Status");
        setReason(HttpStatus.SC_UNPROCESSABLE_ENTITY,
                  "Unprocessable Entity");
        setReason(HttpStatus.SC_INSUFFICIENT_SPACE_ON_RESOURCE,
                  "Insufficient Space On Resource");
        setReason(HttpStatus.SC_METHOD_FAILURE,
                  "Method Failure");
        setReason(HttpStatus.SC_LOCKED,
                  "Locked");
        setReason(HttpStatus.SC_INSUFFICIENT_STORAGE,
                  "Insufficient Storage");
        setReason(HttpStatus.SC_FAILED_DEPENDENCY,
                  "Failed Dependency");
    }


}
