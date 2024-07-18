# summarizefiles

A simple utility to summarize files by size:

---
    2024-07-18 07:07 /home
    
    |        so:   1018.732 MB in 1622 files|       exe:     14.945 MB in 124 file
    |      html:    525.330 MB in 43170 file|        db:     14.398 MB in 21 files
    |  snapshot:    410.956 MB in 12 files  |       xml:     14.073 MB in 930 file
    |         1:    398.707 MB in 1318 files|        ts:     13.688 MB in 3343 fil
    |         h:    265.605 MB in 23087 file|     woff2:     12.498 MB in 248 file
    |        py:    247.999 MB in 25525 file|       sym:     12.191 MB in 3 files 
---

by time modified:

---
    2024-07-18 08:07 /home
    
    |  20240708:      2.622 GB in 17323 files                                     
    |  20240701:    227.300 KB in 2 files                                         
    |  20240628:     17.073 MB in 7 files                                         
    |  20240626:      1.711 MB in 171 files                                       
    |  20240619:      2.507 MB in 92 files                                        
    |    202406:      1.885 GB in 35141 files                                     
    |    202405:    800.992 MB in 28183 files                                     
    |    202404:    233.138 MB in 12998 files                                    
---

by lines of text:

---
    2024-07-18 08:07 /user/rseward/src/play/
    
    |        js: 1631072 lines in 10268 file|       csv:  162 lines in 9 files    
    |        ts: 230967 lines in 1694 files |       sh~:  158 lines in 9 files    
    |        md: 120680 lines in 833 files  |       bnf:  128 lines in 8 files    
    |          : 93440 lines in 2283 files  |        ui:  121 lines in 1 files    
    |       xml: 57084 lines in 20 files    |        jl:  119 lines in 3 files    
    |       css: 52996 lines in 116 files   |     jison:  111 lines in 1 files    
---

I personally use the tool to verify the transfer of files after rsync or the restore of data
from a backup. The tool is also useful for observing recent modifications to a directory tree.

## Building the project

### libcfu

The project incorporates libcfu's hashtable implementation. You will need to build that project
and add libcfu.so to your LD_LIBRARY_PATH to use this tool.

Obtain the source for libcfu here:
- https://github.com/codebrainz/libcfu

### summarizefiles

On Fedora you will need to install the following packages:

- @development-tools for normal make, gcc build tools
- file-devel for libmagic header files and libraries

---
    dnf install @development-tools file-devel
---

After the prereqs are installed.

---
    make
---

