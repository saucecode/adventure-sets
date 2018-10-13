# adventure-sets
My implementation and file format for parsing nested adventure sets. Inspired by the xkcd comic: [Calendar Facts](https://xkcd.com/1930/).

It's written in C, and has no dependencies! I have no idea what this structure is called, so I'm calling them nested adventure sets.

## How to use

    $ gcc -o nas adventure_sets.c
    $ ./nas xkcd-1930
    Did you know that Shark Week happens at the wrong time every year because of a decree by The Pope in the 1500s? Apparently scientists are really worried.

## File Format

The file format consists of sets, choices, and strings, which the program parses into symbols, then 'flattens' into an output string. Here's a sample input

    {
        "The quick "
        { "brown" | "pink" }
        " fox jumped over the lazy "
        { "dog" | "cat"}
    }

which is internally reduced to

    SET[
        STRING, CHOICE[STRING, STRING], STRING, CHOICE[STRING, STRING]
    ]

then can be recursively flattened into any of 4 possible outputs, such as

    The quick pink fox jumped over the lazy dog.

## Fun Internals

The entire list of nested sets of sets can be represented by a single recursive data structure.

    struct symbol_t {
        int type;
        int elements;
        void *data;
    };

The symbol can represent either a SET or a STRING. If it represents a string, then `data` is a regular `char*` C-string, otherwise it is an array of `symbol_t` structs (`elements` is the length of this array). `type` declares if the struct contains a SET, CHOICE, or a STRING.

A CHOICE is a special case of SET. During the "reduction" stage, a SET is reduced to a STRING of all of its elements concatenated, i.e

    { "Drink" " your " "ovaltine" } => "Drink your ovaltine"
    SET[STRING, STRING, STRING]     => STRING

whereas the CHOICE discards all by one of its elements, i.e

    { {"He loves me" | "He loves me not"} | "That'll do, Donkey. That'll do." }
                                           => "He loves me not"
    CHOICE[CHOICE[STRING, STRING], STRING] => CHOICE[STRING, STRING] => STRING


