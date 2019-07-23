






































package netscape.javascript;






public
class JSException extends RuntimeException {
    public static final int EXCEPTION_TYPE_EMPTY = -1;
    public static final int EXCEPTION_TYPE_VOID = 0;
    public static final int EXCEPTION_TYPE_OBJECT = 1;
    public static final int EXCEPTION_TYPE_FUNCTION = 2;
    public static final int EXCEPTION_TYPE_STRING = 3;
    public static final int EXCEPTION_TYPE_NUMBER = 4;
    public static final int EXCEPTION_TYPE_BOOLEAN = 5;
    public static final int EXCEPTION_TYPE_ERROR = 6;

    String filename;
    int lineno;
    String source;
    int tokenIndex;
    private int wrappedExceptionType;
    private Object wrappedException;

    





    public JSException() {
	super();
        filename = "unknown";
        lineno = 0;
        source = "";
        tokenIndex = 0;
	wrappedExceptionType = EXCEPTION_TYPE_EMPTY;
    }

    






    public JSException(String s) {
	super(s);
        filename = "unknown";
        lineno = 0;
        source = "";
        tokenIndex = 0;
	wrappedExceptionType = EXCEPTION_TYPE_EMPTY;
    }

    




    private JSException(int wrappedExceptionType, Object wrappedException) {
	super();
	this.wrappedExceptionType = wrappedExceptionType;
	this.wrappedException = wrappedException;
    }
    
    






    public JSException(String s, String filename, int lineno,
                       String source, int tokenIndex) {
	super(s);
        this.filename = filename;
        this.lineno = lineno;
        this.source = source;
        this.tokenIndex = tokenIndex;
	wrappedExceptionType = EXCEPTION_TYPE_EMPTY;
    }

    



    public int getWrappedExceptionType() {
	return wrappedExceptionType;
    }

    


    public Object getWrappedException() {
	return wrappedException;
    }

}

