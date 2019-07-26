





const START_REQUEST = 0x01;
const STOP_REQUEST = 0x02;
const DATA_AVAILABLE = 0x04;



const SIZE_AVAILABLE = 0x01;
const FRAME_UPDATE = 0x02;
const FRAME_COMPLETE = 0x04;
const LOAD_COMPLETE = 0x08;
const DECODE_COMPLETE = 0x10;



function ImageListener(start_callback, stop_callback)
{
  this.sizeAvailable = function onSizeAvailable(aRequest)
  {
    do_check_false(this.synchronous);

    this.state |= SIZE_AVAILABLE;

    if (this.start_callback)
      this.start_callback(this, aRequest);
  }
  this.frameComplete = function onFrameComplete(aRequest)
  {
    do_check_false(this.synchronous);

    this.state |= FRAME_COMPLETE;
  }
  this.decodeComplete = function onDecodeComplete(aRequest)
  {
    do_check_false(this.synchronous);

    this.state |= DECODE_COMPLETE;
  }
  this.loadComplete = function onLoadcomplete(aRequest)
  {
    do_check_false(this.synchronous);

    try {
      aRequest.requestDecode();
    } catch (e) {
      do_print("requestDecode threw " + e);
    }

    this.state |= LOAD_COMPLETE;

    if (this.stop_callback)
      this.stop_callback(this, aRequest);
  }
  this.frameUpdate = function onFrameUpdate(aRequest)
  {
  }
  this.isAnimated = function onIsAnimated()
  {
  }

  
  
  this.synchronous = true;

  
  this.start_callback = start_callback;

  
  this.stop_callback = stop_callback;

  
  
  
  this.state = 0;
}

function NS_FAILED(val)
{
  return !!(val & 0x80000000);
}

function ChannelListener()
{
  this.onStartRequest = function onStartRequest(aRequest, aContext)
  {
    if (this.outputListener)
      this.outputListener.onStartRequest(aRequest, aContext);

    this.requestStatus |= START_REQUEST;
  }

  this.onDataAvailable = function onDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount)
  {
    if (this.outputListener)
      this.outputListener.onDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount);

    this.requestStatus |= DATA_AVAILABLE;
  }

  this.onStopRequest = function onStopRequest(aRequest, aContext, aStatusCode)
  {
    if (this.outputListener)
      this.outputListener.onStopRequest(aRequest, aContext, aStatusCode);

    
    
    if (NS_FAILED(aStatusCode))
      this.requestStatus = 0;
    else
      this.requestStatus |= STOP_REQUEST;
  }

  
  this.outputListener = null;

  
  
  
  this.requestStatus = 0;
}
