
# Syntax

```rust
use valk:mem
use valk:mem { alloc, free as {alias}, copy }

global {name} : int (0) // thread local global (recommended)
shared {name} : int (1) // shared global
~ shared {name} : int (2) // readonly shared global, public in this file
- const {name} : {type} ({value}) // private constant value

// Struct
struct A[T] is MyInterface, MyInterface2 {
    // Properties
    {name} : {type} [({default-value})] // public everywhere
    - {name} : {type} [({default-value})] // private, public in this file
    -ns {name} : {type} [({default-value})] // private, public in this namespace
    -pkg {name} : {type} [({default-value})] // private, public in this package
    ~ {name} : {type} [({default-value})] // readonly, public in this file
    ~ns {name} : {type} [({default-value})] // readonly, public in this namespace
    ~pkg {name} : {type} [({default-value})] // readonly, public in this package

    // Traits
    use {trait-name}
    use {class-name} // use other class as a trait

    // Static function
    static fn {name}(arg: int (100)) String { ... }

    // Public function
    fn {name}() { ... }

    // Function meta info
    #display_name "{display-name}"
    #description "A function that..."
    inline fn {name}() { ... }

    global {name} : {type} ({default-value}) // define a global in this struct, aka. static property
    shared {name} : {type} ({default-value}) // define a shared global in this struct, aka. shared static property
    enum {name} : {type} {
        NAME1
        NAME2 ({value})
        NAME3
    }
}

type {name} : {type} // Type alias
value {name} ({value}) // Value alias

// Function
fn alloc(size: uint (100)) ptr {
    // Inline function 1
    def fn myfunc() { ... code ... }
    myfunc()
    // Inline function 2 (closure)
    let myfunc = fn () { ... code ... }
    myfunc()
    // Inline struct
    def struct myStruct { ... props & functions ... }
    // Inline global
    def global g1 : int (10)
    // Inline enum
    def enum e1 : {type} { ... values ... }
    return mem:alloc(size)
}

enum {type} : {name} {
    NAME1 // auto value (if possible)
    NAME2 = {value}
    NAME3 // auto value
}
```
