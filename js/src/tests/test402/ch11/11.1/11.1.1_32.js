








var read = 0;

function readMinimumSignificantDigits() {
    ++read;
    if (read === 1) {
        return 0; 
    } else if (read === 3) {
        return 1; 
    } else {
        $ERROR("minimumSignificantDigits read out of sequence: " + read + ".");
    }
}

function readMaximumSignificantDigits() {
    ++read;
    if (read === 2) {
        return 0; 
    } else if (read === 4) {
        return 1; 
    } else {
        $ERROR("maximumSignificantDigits read out of sequence: " + read + ".");
    }
}

var options = {};
Object.defineProperty(options, "minimumSignificantDigits",
    { get: readMinimumSignificantDigits });
Object.defineProperty(options, "maximumSignificantDigits",
    { get: readMaximumSignificantDigits });

new Intl.NumberFormat("de", options);

if (read !== 4) {
    $ERROR("insuffient number of property reads: " + read + ".");
}
