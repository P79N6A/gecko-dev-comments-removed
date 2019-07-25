


























package ch.boye.httpclientandroidlib;







public class MethodNotSupportedException extends HttpException {

    private static final long serialVersionUID = 3365359036840171201L;

    




    public MethodNotSupportedException(final String message) {
        super(message);
    }

    






    public MethodNotSupportedException(final String message, final Throwable cause) {
        super(message, cause);
    }
}
