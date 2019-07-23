




































package nu.validator.htmlparser.cpptranslate;

import java.io.IOException;
import java.io.InputStream;

public class NoCppInputStream extends InputStream {

    private final static char[] START = "[NOCPP[".toCharArray();
    
    private final static char[] END = "]NOCPP]".toCharArray();
    
    private int state;
    
    private final InputStream delegate;
    
    
    
    


    public NoCppInputStream(InputStream delegate) {
        this.delegate = delegate;
        this.state = 0;
    }

    @Override public int read() throws IOException {
        int c;
        if (state == START.length) {
            int endState = 0;
            while (endState != END.length) {
                c = delegate.read();
                if (END[endState] == c) {
                    endState++;
                } else {
                    endState = 0;
                }
            }
            state = 0;
        }
        c = delegate.read();
        if (START[state] == c) {
            state++;
        } else {
            state = 0;
        }
        return c;
    }

}
