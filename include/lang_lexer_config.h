#define STATIC_ARRAY(NAME, TYPE, ...)      \
    static const TYPE NAME[] = { __VA_ARGS__ };  \
    static int NAME##_size = sizeof(NAME) / sizeof(TYPE)



STATIC_ARRAY(lang_keywords, char*,
    "void",
    "const",
    "static",
    "char",
    "int",
    "while",
    "if"
);

STATIC_ARRAY(lang_separators, char,
    '{',
    '}',
    '(',
    ')',
    ';',
    ','
);

STATIC_ARRAY(lang_operators, char*,     
    "=",
    "+",
    "-",
    "*",
    "%",
    ">",
    "<",
);

STATIC_ARRAY(lang_number_literal_chars, char,
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '.',
);
