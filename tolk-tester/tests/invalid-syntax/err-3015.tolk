fun main() {
    return ion(123);
}

/**
@compilation_should_fail
@stderr function `ion` requires a constant string, like `ion("0.05")`
 */
