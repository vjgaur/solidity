function id(uint x) pure returns (uint) {
    return x;
}

function zero(address) pure returns (address) {
    return address(0);
}

contract C {
    using * for *;
    function f(uint x) pure external returns (uint) {
        return x.id();
    }
    function g(address a) pure external returns (address) {
        return a.zero();
    }
}
// ====
// compileViaYul: also
// ----
// f(uint256): 5 -> 5
// f(uint256): 10 -> 10
// g(address): 5 -> 0
// g(address): 10 -> 0
