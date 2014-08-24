pg-search-thai
============================

pg-search-thai is Thai Full Text Search extension for _PostgreSQL_.

The main purpuse is to:

Use PostgreSQL Full Text Search for Thai language (which spaces were not used to separate words )

This extension requires Thai word breaking routine in LibThai, libiconv for character encoding conversion (that requires by LibThai word breaking as it can only do tis-620 character encoding word segmentation) and
a pre-installed postgresql which C compiler will use to find `pg_config`.
If LibThai is already not installed on your system, please [check out](http://linux.thai.net/projects/libthai) or Installation section for more details

##Installation

- Download the [_libthai_](http://linux.thai.net/projects/libthai) (and its dependency: [_libdatrie_](http://linux.thai.net/~thep/datrie/datrie.html#Download) ).

- Download the [_libiconv_](https://www.gnu.org/software/libiconv/).

- Install _libthai_ and _libiconv_ on your local system.

- Install the extension, at project root directory, you can simply run:

     ````make all
     ````
- But, if you would like to just install the thai parser, just go into thai_parser directory, compile and install it, like so:

     ````cd thai_parser; make; make install
     ````

##Usage

- Start up **psql** and type:

     ```CREATE EXTENSION thai_parser;
     CREATE TEXT SEARCH CONFIGURATION thaicfg (PARSER = thai_parser);
     ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple;
     ```

##Example 1
Check how parser works.

    ```select * from ts_parse('thai_parser', 'ต้มยำกุ้งน้ำข้น ( Thai sour and spicy shrimp soup ) และไข่เจียวร้อนๆ');
    ```

##Example 2
Try to build document from `thaicfg` configuration that uses the specified parser.

    ```select to_tsvector('thaicfg', 'ต้มยำกุ้งน้ำข้น ( Thai sour and spicy shrimp soup ) และไข่เจียวร้อนๆ');
    ```

##Example 3
Querying

    ````gdsire=# select to_tsvector('testthaicfg', 'the land of somtum (ส้มตำ)') @@ to_tsquery('testthaicfg','ส้มตำ');
     ?column?
    ----------
     t
    (1 row)
    ````
##Example 4
Querying with `|` and `&` operator.

    ````
    gdsire=#  select to_tsvector('testthaicfg', 'ส้มตำไก่ย่าง ต้มยำกุ้ง in thailand') @@ to_tsquery('testthaicfg','ข้าวเหนียว&ส้มตำ');
     ?column?
    ----------
     f
    (1 row)

    gdsire=#  select to_tsvector('testthaicfg', 'ข้าวเหนียวส้มตำไก่ย่าง ต้มยำกุ้ง in thailand') @@ to_tsquery('testthaicfg','ข้าวเหนียว&ส้มตำ');
     ?column?
    ----------
     t
    (1 row)
    ````

##Example 5
 If you want to use hunspell as a dictionary for the full text search.
 Make sure you have already install thai hunspell dictionay files in `pg_config --sharedir`/tsearch_data directory.

    ````
    CREATE TEXT SEARCH DICTIONARY thai_hunspell (
        TEMPLATE = ispell,
        DictFile = th_TH,
        AffFile = th_TH,
        StopWords = english
    );
    ````

In psql console type `\dFd` to see if dictionary is installed.
Then,

    ````ALTER TEXT SEARCH CONFIGURATION testthaicfg ADD MAPPING FOR a WITH simple, thai_hunspell;````
   
Test

    ````SELECT ts_lexize('thai_hunspell', 'ทดสอบ');````


_pg-search-thai_ is released under the GNU General Public License (GPLv2).
Refer to License [FAQ](http://www.gnu.org/licenses/old-licenses/gpl-2.0-faq.html) for more information.
_pg-search-thai_ is developed by Di Warachet (zdk).
