




function Reflect_has(target, propertyKey) {
    
    if (!IsObject(target))
        ThrowTypeError(JSMSG_NOT_NONNULL_OBJECT, DecompileArg(0, target));

    
    return propertyKey in target;
}
