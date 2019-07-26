










function ComputeSlicesInfo(length) {
  var count = length >>> MAX_SLICE_SHIFT;
  var numWorkers = ForkJoinNumWorkers();
  if (count < numWorkers)
    count = numWorkers;
  else if (count >= numWorkers * MAX_SLICES_PER_WORKER)
    count = numWorkers * MAX_SLICES_PER_WORKER;

  
  var shift = std_Math_max(std_Math_log2(length / count) | 0, 1);

  
  count = length >>> shift;
  if (count << shift !== length)
    count += 1;

  return { shift: shift, statuses: new Uint8Array(count), lastSequentialId: 0 };
}




function SlicesInfoClearStatuses(info) {
  var statuses = info.statuses;
  var length = statuses.length;
  for (var i = 0; i < length; i++)
    UnsafePutElements(statuses, i, 0);
  info.lastSequentialId = 0;
}





function NextSequentialSliceId(info, doneMarker) {
  var statuses = info.statuses;
  var length = statuses.length;
  for (var i = info.lastSequentialId; i < length; i++) {
    if (statuses[i] === SLICE_STATUS_DONE)
      continue;
    info.lastSequentialId = i;
    return i;
  }
  return doneMarker == undefined ? length : doneMarker;
}




function ShrinkLeftmost(info) {
  return function () {
    return [NextSequentialSliceId(info), SLICE_COUNT(info)]
  };
}


