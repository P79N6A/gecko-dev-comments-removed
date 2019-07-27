







function extractValue({
  objectName,
  object,
  property,
  expectedType,
  trim
}, console) {
  const value = object[property];
  const isArray = Array.isArray(value);
  
  const type = (isArray) ? 'array' : typeof value;
  if (type !== expectedType) {
    if (type !== 'undefined') {
      let msg = `Expected the ${objectName}'s ${property} `;
      msg += `member to be a ${expectedType}.`;
      console.log(msg);
    }
    return undefined;
  }
  
  const shouldTrim = expectedType === 'string' && value && trim;
  if (shouldTrim) {
    return value.trim() || undefined;
  }
  return value;
}
