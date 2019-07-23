







































package netscape.javascript;
import java.io.*;

public class JSUtil {

    
    public static String getStackTrace(Throwable t) {
	ByteArrayOutputStream captureStream;
	PrintWriter p;
	
	captureStream = new ByteArrayOutputStream();
	p = new PrintWriter(captureStream);

	t.printStackTrace(p);
	p.flush();

	return captureStream.toString();
    }
}
