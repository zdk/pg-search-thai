pg-thai-textsearch-extension
============================

_pg-thai-textsearch-extension_ is Thai Full Text Search extension for _PostgreSQL_.

The main purpuse is to:

Use PostgreSQL Full Text Search for Thai language (which spaces were not used to separate words )

This extension requires segmentation system installed LibThai (LibThai does not require source code), libiconv for character encoding conversion (requires by LibThai word break), a pre-installed postgresql which C compiler will use to find `pg_config`. If LibThai is not installed on your system, please check out http://linux.thai.net/projects/libthai for more details

##Configuration and Installation

- Download the _libthai_ (its dependency: _libdatrie_ ) from the following link: http://linux.thai.net/projects/libthai

- Download the _libiconv_ from the following link: https://www.gnu.org/software/libiconv/

- Install _libthai_ and _libiconv_ on your local system.

- Install the extension

```Shell
cd thai_parser; make; make install
```

##Usage

- Start up **psql** and type:

```Sql
CREATE EXTENSION thai_parser;
CREATE TEXT SEARCH CONFIGURATION thaicfg (PARSER = thai_parser);
ALTER TEXT SEARCH CONFIGURATION thaicfg ADD MAPPING FOR a WITH simple;
```

##Example

```Sql
select * from ts_parse('thai_parser', 'อาหารไทย ต้มยำกุ้งน้ำข้น (Thai sour and spicy shrimp soup) รสเด็ด');
```