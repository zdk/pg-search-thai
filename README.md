pg-search-thai-extension
============================

pg-search-thai-extension is Thai Full Text Search extension for _PostgreSQL_.

The main purpuse is to:

Use PostgreSQL Full Text Search for Thai language (which spaces were not used to separate words )

This extension requires Thai word breaking routine in LibThai, libiconv for character encoding conversion (that requires by LibThai word breaking as it can only do tis-620 character encoding word segmentation) and
a pre-installed postgresql which C compiler will use to find `pg_config`.
If LibThai is already not installed on your system, please check out http://linux.thai.net/projects/libthai or Installation section for more details

##Installation

- Download the _libthai_ (its dependency: _libdatrie_ ) from the following link: http://linux.thai.net/projects/libthai

- Download the _libiconv_ from the following link: https://www.gnu.org/software/libiconv/

- Install _libthai_ and _libiconv_ on your local system.

- Install the extension

     ```cd thai_parser; make; make install
     ```

##Usage

- Start up **psql** and type:

     ```CREATE EXTENSION thai_parser;
     CREATE TEXT SEARCH CONFIGURATION thaicfg (PARSER = thai_parser);
     ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple;
     ```

##Example 1
Check how parser works.

    ```select * from ts_parse('thai_parser', 'ต้มยำกุ้งน้ำข้น (Thai sour and spicy shrimp soup) อร่อยเข้มข้น');
    ```

##Example 2
Try to build document from `thaicfg` configuration that uses the specified parser.

    ```select to_tsvector('thaicfg', 'ต้มยำกุ้งน้ำข้น (Thai sour and spicy shrimp soup) อร่อยเข้มข้น');
    ```

##Example 3

Querying

````gdsire=# select to_tsvector('testthaicfg', 'the land of somtum (ส้มตำ)') @@ to_tsquery('testthaicfg','ส้มตำ');
 ?column?
----------
 t
(1 row)
````