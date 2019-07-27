



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

function getInflatedStackLocations(thread, sample) {
  let stackTable = thread.stackTable;
  let frameTable = thread.frameTable;
  let stringTable = thread.stringTable;
  let SAMPLE_STACK_SLOT = thread.samples.schema.stack;
  let STACK_PREFIX_SLOT = stackTable.schema.prefix;
  let STACK_FRAME_SLOT = stackTable.schema.frame;
  let FRAME_LOCATION_SLOT = frameTable.schema.location;

  
  
  let stackIndex = sample[SAMPLE_STACK_SLOT];
  let locations = [];
  while (stackIndex !== null) {
    let stackEntry = stackTable.data[stackIndex];
    let frame = frameTable.data[stackEntry[STACK_FRAME_SLOT]];
    locations.push(stringTable[frame[FRAME_LOCATION_SLOT]]);
    stackIndex = stackEntry[STACK_PREFIX_SLOT];
  }

  
  return locations.reverse();
}
