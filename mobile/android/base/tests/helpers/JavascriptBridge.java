



package org.mozilla.gecko.tests.helpers;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import junit.framework.AssertionFailedError;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Actions.EventExpecter;
import org.mozilla.gecko.Assert;
import org.mozilla.gecko.tests.UITestContext;





































public final class JavascriptBridge {

    private static enum MessageStatus {
        QUEUE_EMPTY, 
        PROCESSED,   
        REPLIED,     
        SAVED,       
    };

    @SuppressWarnings("serial")
    public static class CallException extends RuntimeException {
        public CallException() {
            super();
        }

        public CallException(final String msg) {
            super(msg);
        }

        public CallException(final String msg, final Throwable e) {
            super(msg, e);
        }

        public CallException(final Throwable e) {
            super(e);
        }
    }

    public static final String EVENT_TYPE = "Robocop:JS";

    private static Actions sActions;
    private static Assert sAsserter;

    
    private final Object mTarget;
    
    private final Method[] mMethods;
    
    private final JavascriptMessageParser mLogParser;
    
    private final EventExpecter mExpecter;
    
    private JSONObject mSavedAsyncMessage;
    
    private int mCallStackDepth;
    
    private boolean mJavaBridgeLoaded;

     static void init(final UITestContext context) {
        sActions = context.getActions();
        sAsserter = context.getAsserter();
    }

    public JavascriptBridge(final Object target) {
        mTarget = target;
        mMethods = target.getClass().getMethods();
        mLogParser = new JavascriptMessageParser(sAsserter);
        mExpecter = sActions.expectGeckoEvent(EVENT_TYPE);
    }

    






    public void syncCall(final String method, final Object... args) {
        mCallStackDepth++;

        sendMessage("sync-call", method, args);
        try {
            while (processPendingMessage() != MessageStatus.REPLIED) {
            }
        } catch (final AssertionFailedError e) {
            
            throw new CallException("Cannot call " + method, e);
        }

        
        
        
        if (mCallStackDepth == 1) {
            
            
            finishPendingCalls();
        }
        mCallStackDepth--;
    }

    






    public void asyncCall(final String method, final Object... args) {
        sendMessage("async-call", method, args);
    }

    


    public void disconnect() {
        mExpecter.unregisterListener();
    }

    




    private MessageStatus processPendingMessage() {
        
        
        
        
        try {
            final String message = mExpecter.blockForEventData();
            return processMessage(new JSONObject(message));
        } catch (final JSONException e) {
            throw new IllegalStateException("Invalid message", e);
        }
    }

    




    private MessageStatus maybeProcessPendingMessage() {
        
        final String message = mExpecter.blockForEventDataWithTimeout(0);
        if (message != null) {
            try {
                return processMessage(new JSONObject(message));
            } catch (final JSONException e) {
                throw new IllegalStateException("Invalid message", e);
            }
        }
        if (mSavedAsyncMessage != null) {
            
            return processMessage(mSavedAsyncMessage);
        }
        return MessageStatus.QUEUE_EMPTY;
    }

    


    private void finishPendingCalls() {
        MessageStatus result;
        do {
            result = maybeProcessPendingMessage();
            if (result == MessageStatus.REPLIED) {
                throw new IllegalStateException("Sync reply was unexpected");
            }
        } while (result != MessageStatus.QUEUE_EMPTY);
    }

    private void ensureJavaBridgeLoaded() {
        while (!mJavaBridgeLoaded) {
            processPendingMessage();
        }
    }

    private void sendMessage(final String innerType, final String method, final Object[] args) {
        ensureJavaBridgeLoaded();

        
        final JSONObject message = new JSONObject();
        final JSONArray jsonArgs = new JSONArray();
        try {
            if (args != null) {
                for (final Object arg : args) {
                    jsonArgs.put(convertToJSONValue(arg));
                }
            }
            message.put("type", EVENT_TYPE)
                   .put("innerType", innerType)
                   .put("method", method)
                   .put("args", jsonArgs);
        } catch (final JSONException e) {
            throw new IllegalStateException("Unable to create JSON message", e);
        }
        sActions.sendGeckoEvent(EVENT_TYPE, message.toString());
    }

    private MessageStatus processMessage(JSONObject message) {
        final String type;
        final String methodName;
        final JSONArray argsArray;
        final Object[] args;
        try {
            if (!EVENT_TYPE.equals(message.getString("type"))) {
                throw new IllegalStateException("Message type is not " + EVENT_TYPE);
            }
            type = message.getString("innerType");

            if ("progress".equals(type)) {
                
                mLogParser.logMessage(message.getString("message"));
                return MessageStatus.PROCESSED;

            } else if ("notify-loaded".equals(type)) {
                mJavaBridgeLoaded = true;
                return MessageStatus.PROCESSED;

            } else if ("sync-reply".equals(type)) {
                
                return MessageStatus.REPLIED;

            } else if ("sync-call".equals(type) || "async-call".equals(type)) {

                if ("async-call".equals(type)) {
                    
                    
                    
                    
                    
                    
                    final JSONObject newSavedMessage =
                        (message != mSavedAsyncMessage ? message : null);
                    message = mSavedAsyncMessage;
                    mSavedAsyncMessage = newSavedMessage;
                    if (message == null) {
                        
                        return MessageStatus.SAVED;
                    }
                }

                methodName = message.getString("method");
                argsArray = message.getJSONArray("args");
                args = new Object[argsArray.length()];
                for (int i = 0; i < args.length; i++) {
                    args[i] = convertFromJSONValue(argsArray.get(i));
                }
                invokeMethod(methodName, args);

                if ("sync-call".equals(type)) {
                    
                    sendMessage("sync-reply", methodName, null);
                }
                return MessageStatus.PROCESSED;
            }
            throw new IllegalStateException("Message type is unexpected");

        } catch (final JSONException e) {
            throw new IllegalStateException("Unable to retrieve JSON message", e);
        }
    }

    



    private Object invokeMethod(final String methodName, final Object[] args) {
        final Class<?>[] argTypes = new Class<?>[args.length];
        for (int i = 0; i < argTypes.length; i++) {
            if (args[i] == null) {
                argTypes[i] = Object.class;
            } else {
                argTypes[i] = args[i].getClass();
            }
        }

        
        try {
            return invokeMethod(mTarget.getClass().getMethod(methodName, argTypes), args);
        } catch (final NoSuchMethodException e) {
            
        }

        
        
        
        
        
        
        Throwable lastException = null;
        for (final Method method : mMethods) {
            if (!method.getName().equals(methodName)) {
                continue;
            }
            try {
                return invokeMethod(method, args);
            } catch (final IllegalArgumentException e) {
                lastException = e;
                
            } catch (final UnsupportedOperationException e) {
                
                lastException = e;
                
            }
        }
        
        throw new UnsupportedOperationException(
            "Cannot call method " + methodName + " (not public? wrong argument types?)",
            lastException);
    }

    private Object invokeMethod(final Method method, final Object[] args) {
        try {
            return method.invoke(mTarget, args);
        } catch (final IllegalAccessException e) {
            throw new UnsupportedOperationException(
                "Cannot access method " + method.getName(), e);
        } catch (final InvocationTargetException e) {
            final Throwable cause = e.getCause();
            if (cause instanceof CallException) {
                
                
                throw (CallException) cause;
            }
            throw new CallException("Failed to invoke " + method.getName(), cause);
        }
    }

    private Object convertFromJSONValue(final Object value) {
        if (value == JSONObject.NULL) {
            return null;
        }
        return value;
    }

    private Object convertToJSONValue(final Object value) {
        if (value == null) {
            return JSONObject.NULL;
        }
        return value;
    }
}
