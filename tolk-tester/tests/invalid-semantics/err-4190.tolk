struct A {
    ok: coins = ion("0.05"),
    err: int = blockchain.now(),
}

/**
@compilation_should_fail
@stderr not a constant expression
@stderr err: int
 */
