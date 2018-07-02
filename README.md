pg-search-thai
============================

pg-search-thai is full text search _PostgreSQL_ extension for Thai Language.

Its main purpose is:

To enable PostgreSQL Full Text Search in Thai language (Due to Thai Language does not use spaces to separate words)

## Prerequisite

libthai, libiconv - this pg extension requires Thai word breaking functionality from the popular `LibThai` and libiconv.
postgresql - `pg_config` in order to build this extension.

## Installation

- Download the [_libthai_](http://linux.thai.net/projects/libthai) (and its dependency: [_libdatrie_](http://linux.thai.net/~thep/datrie/datrie.html#Download) ).

- Download the [_libiconv_](https://www.gnu.org/software/libiconv/).

- Install _libthai_ and _libiconv_ on your local system.

- Install the extension from source, go to project root directory (`cd pg-search-thai`). Then, you can simply run:

     ```make all```

- If you would like to install only the thai parser, just go into thai_parser directory. Then, compile and install it, like so:

     ```cd thai_parser; make; make install```

## Usage

- Start the **psql** console ( Or any postgresql client, **pgAdmin** for instance ) and create the extension you have just installed by typing following commands:

    ```CREATE EXTENSION thai_parser;```

    ```CREATE TEXT SEARCH CONFIGURATION thaicfg (PARSER = thai_parser);```

    ```ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple;```

- Note: This extension is only tested with `UTF-8` encoding. So, it is highly recommended to initial database with utf-8.

## Example 1
Check how parser works.

    SELECT * FROM ts_parse('thai_parser', 'ต้มยำกุ้งน้ำข้น ( Thai sour and spicy shrimp soup ) และไข่เจียวร้อนๆ');

## Example 2
Try to build document from `thaicfg` configuration that uses the specified parser.

    SELECT to_tsvector('thaicfg', 'ต้มยำกุ้งน้ำข้น ( Thai sour and spicy shrimp soup ) และไข่เจียวร้อนๆ');

## Example 3
Querying

    SELECT to_tsvector('thaicfg', 'the land of somtum (ส้มตำ)') @@ to_tsquery('thaicfg','ส้มตำ');
     ?column?
    ----------
     t
    (1 row)

## Example 4
Querying with `|` and `&` operator.

    SELECT to_tsvector('thaicfg', 'ส้มตำไก่ย่าง ต้มยำกุ้ง in thailand') @@ to_tsquery('thaicfg','ข้าวเหนียว&ส้มตำ');
     ?column?
    ----------
     f
    (1 row)

    SELECT to_tsvector('thaicfg', 'ข้าวเหนียวส้มตำไก่ย่าง ต้มยำกุ้ง in thailand') @@ to_tsquery('thaicfg','ข้าวเหนียว&ส้มตำ');
     ?column?
    ----------
     t
    (1 row)

## Example 5
 If you want to use hunspell as a dictionary for the full text search.
 Make sure you have already install thai hunspell dictionay files in `pg_config --sharedir`/tsearch_data directory.

    CREATE TEXT SEARCH DICTIONARY thai_hunspell (
        TEMPLATE = ispell,
        DictFile = th_TH,
        AffFile = th_TH,
        StopWords = english
    );


In psql console type `\dFd` to see if dictionary is installed.
Then,

    ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple, thai_hunspell;

And, test with,

    SELECT ts_lexize('thai_hunspell', 'ทดสอบ');

## Bugs Report and Contributing

GitHub issue tracker and pull requests are welcome.

_pg-search-thai_ is released under the GNU General Public License (GPLv2).
Refer to License [FAQ](http://www.gnu.org/licenses/old-licenses/gpl-2.0-faq.html) for more information.
