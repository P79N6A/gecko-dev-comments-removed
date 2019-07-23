






































package netscape.oji;
import java.net.URLClassLoader;
import java.net.URL;
import java.net.MalformedURLException;

public abstract class ProxyClassLoaderFactory {

    public static ClassLoader createClassLoader(final String documentURL) 
                  throws MalformedURLException {
        URL[] documentURLArray = new URL[1];
        int lastIndx = documentURL.lastIndexOf("/");
        String urlPath = documentURL.substring(0, lastIndx+1);
        try {
            documentURLArray[0] = new URL(urlPath);
		}
        catch (MalformedURLException e) {
            System.out.println("MalformedURLException was caught");
            return null;
        }
        return new URLClassLoader(documentURLArray);
    }
}
