

























package ch.boye.httpclientandroidlib.client.cache;

import java.io.IOException;
import java.io.InputStream;







public interface ResourceFactory {

    














    Resource generate(String requestId, InputStream instream, InputLimit limit) throws IOException;

    







    Resource copy(String requestId, Resource resource) throws IOException;

}
