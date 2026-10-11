#pragma once

// Data types of the language plus the internal node kinds used by the symbol tree.
enum DATA_TYPE {
    TYPE_INT = 1,   // int
    TYPE_SHORT_INT, // short
    TYPE_LONG_INT,  // long
    TYPE_BOOL,      // bool
    TYPE_FUNCT,     // function
    TYPE_SCOPE      // lexical scope (symbol table frame)
};
