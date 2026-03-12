fun main(am: slice) {
    return ion(am);
}

/**
@compilation_should_fail
@stderr function `ion` requires a constant string, like `ion("0.05")`
 */
