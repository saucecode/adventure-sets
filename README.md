# adventure-sets
My implementation and file format for parsing nested adventure sets. Inspired by the xkcd comic: [Calendar Facts](https://xkcd.com/1930/). It's written in C, and has no dependencies!

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
