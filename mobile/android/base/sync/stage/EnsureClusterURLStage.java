



package org.mozilla.gecko.sync.stage;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.GeneralSecurityException;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.NodeAuthenticationException;
import org.mozilla.gecko.sync.NullClusterURLException;
import org.mozilla.gecko.sync.ThreadPool;
import org.mozilla.gecko.sync.net.BaseResource;
import org.mozilla.gecko.sync.net.BaseResourceDelegate;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.ClientProtocolException;

public class EnsureClusterURLStage extends AbstractNonRepositorySyncStage {
  public interface ClusterURLFetchDelegate {
    



    public void handleSuccess(URI url);

    



    public void handleThrottled();

    




    public void handleFailure(HttpResponse response);

    


    public void handleError(Exception e);
  }

  protected static final String LOG_TAG = "EnsureClusterURLStage";

  
  
  







  public static void fetchClusterURL(final String nodeWeaveURL,
                                     final ClusterURLFetchDelegate delegate) throws URISyntaxException {
    Logger.info(LOG_TAG, "In fetchClusterURL: node/weave is " + nodeWeaveURL);

    BaseResource resource = new BaseResource(nodeWeaveURL);
    resource.delegate = new BaseResourceDelegate(resource) {

      




















      @Override
      public void handleHttpResponse(HttpResponse response) {
        try {
          int status = response.getStatusLine().getStatusCode();
          switch (status) {
          case 200:
            Logger.info(LOG_TAG, "Got 200 for node/weave cluster URL request (user found; succeeding).");
            HttpEntity entity = response.getEntity();
            if (entity == null) {
              delegate.handleThrottled();
              BaseResource.consumeEntity(response);
              return;
            }
            String output = null;
            try {
              InputStream content = entity.getContent();
              BufferedReader reader = new BufferedReader(new InputStreamReader(content, "UTF-8"), 1024);
              output = reader.readLine();
              BaseResource.consumeReader(reader);
              reader.close();
            } catch (IllegalStateException e) {
              delegate.handleError(e);
              BaseResource.consumeEntity(response);
              return;
            } catch (IOException e) {
              delegate.handleError(e);
              BaseResource.consumeEntity(response);
              return;
            }

            if (output == null || output.equals("null")) {
              delegate.handleThrottled();
              return;
            }

            try {
              URI uri = new URI(output);
              delegate.handleSuccess(uri);
            } catch (URISyntaxException e) {
              delegate.handleError(e);
            }
            break;
          case 400:
          case 404:
            Logger.info(LOG_TAG, "Got " + status + " for node/weave cluster URL request (user not found; failing).");
            delegate.handleFailure(response);
            break;
          case 503:
            Logger.info(LOG_TAG, "Got 503 for node/weave cluster URL request (error fetching node; failing).");
            delegate.handleFailure(response);
            break;
          default:
            Logger.warn(LOG_TAG, "Got " + status + " for node/weave cluster URL request (unexpected HTTP status; failing).");
            delegate.handleFailure(response);
          }
        } finally {
          BaseResource.consumeEntity(response);
        }

        BaseResource.consumeEntity(response);
      }

      @Override
      public void handleHttpProtocolException(ClientProtocolException e) {
        delegate.handleError(e);
      }

      @Override
      public void handleHttpIOException(IOException e) {
        delegate.handleError(e);
      }

      @Override
      public void handleTransportException(GeneralSecurityException e) {
        delegate.handleError(e);
      }
    };

    resource.get();
  }

  public void execute() throws NoSuchStageException {
    final URI oldClusterURL = session.config.getClusterURL();
    final boolean wantNodeAssignment = session.callback.wantNodeAssignment();

    if (!wantNodeAssignment && oldClusterURL != null) {
      Logger.info(LOG_TAG, "Cluster URL is already set and not stale. Continuing with sync.");
      session.advance();
      return;
    }

    Logger.info(LOG_TAG, "Fetching cluster URL.");
    final ClusterURLFetchDelegate delegate = new ClusterURLFetchDelegate() {

      @Override
      public void handleSuccess(final URI url) {
        Logger.info(LOG_TAG, "Node assignment pointed us to " + url);

        if (oldClusterURL != null && oldClusterURL.equals(url)) {
          
          session.callback.informNodeAuthenticationFailed(session, url);
          session.abort(new NodeAuthenticationException(), "User password has changed.");
          return;
        }

        session.callback.informNodeAssigned(session, oldClusterURL, url); 
        session.config.setClusterURL(url);

        ThreadPool.run(new Runnable() {
          @Override
          public void run() {
            session.advance();
          }
        });
      }

      @Override
      public void handleThrottled() {
        session.abort(new NullClusterURLException(), "Got 'null' cluster URL. Aborting.");
      }

      @Override
      public void handleFailure(HttpResponse response) {
        int statusCode = response.getStatusLine().getStatusCode();
        Logger.warn(LOG_TAG, "Got HTTP failure fetching node assignment: " + statusCode);
        if (statusCode == 404) {
          URI serverURL = session.config.serverURL;
          if (serverURL != null) {
            Logger.info(LOG_TAG, "Using serverURL <" + serverURL.toASCIIString() + "> as clusterURL.");
            session.config.setClusterURL(serverURL);
            session.advance();
            return;
          }
          Logger.warn(LOG_TAG, "No serverURL set to use as fallback cluster URL. Aborting sync.");
          
        } else {
          session.interpretHTTPFailure(response);
        }
        session.abort(new Exception("HTTP failure."), "Got failure fetching cluster URL.");
      }

      @Override
      public void handleError(Exception e) {
        session.abort(e, "Got exception fetching cluster URL.");
      }
    };

    ThreadPool.run(new Runnable() {
      @Override
      public void run() {
        try {
          fetchClusterURL(session.config.nodeWeaveURL(), delegate);
        } catch (URISyntaxException e) {
          session.abort(e, "Invalid URL for node/weave.");
        }
      }});
  }
}
