fun get_incoming_value() { return 3; }

fun main() {
    var incoming_ion: int = get_incoming_value().3();
}

/**
@compilation_should_fail
@stderr method `3` not found
 */
