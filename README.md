dbi
===
  database interface commands for Tcl
  by Peter De Rijk (VIB - UAntwerp Center for Molecular Neurology) 

What is dbi
-----------

The dbi interface is a generic Tcl [interface](https://github.com/derijkp/tcl-interface)
for accessing different SQL databases.
It presents a generic api to open, query, and change databases.
The dbi package contains the definition of the 
[dbi interface](https://derijkp.github.io/tcl-dbi/html/interface_dbi.html)
and the [dbi admin interface](https://derijkp.github.io/tcl-dbi/html/interface_dbi_admin.html)
and some tools.

The following implementations are included in the dbi distribution (some
of which have their own extra commands):

* [Firebird](https://derijkp.github.io/tcl-dbi/dbi_firebird.html)
* [sqlite3](https://derijkp.github.io/tcl-dbi/dbi_sqlite3.html)
* [sqlite](https://derijkp.github.io/tcl-dbi/dbi_sqlite.html)
* [postgresql](https://derijkp.github.io/tcl-dbi/dbi_postgresql.html)
* [mysql](https://derijkp.github.io/tcl-dbi/dbi_mysql.html)
* [odbc](https://derijkp.github.io/tcl-dbi/dbi_odbc.html)

Installation
------------
You should be able to obtain the latest version of dbi via www on url
https://derijkp.github.io/tcl-dbi

Compiled packages should be created using the following steps in the package directory:
(You can also build in any other directory, if you change the path to the configure command)
./configure
make
make install

Some of the make targets (such as install) need a working Tcl, and a package called pkgtools,
also available from https://github.com/derijkp/pkgtools

The configure command has several options that can be examined using
/configure --help

The dbi package contains directories with interfaces to several databases. The
ones you need should also be compiled in a similar way to the basic dbi package

Portable build
--------------
The package also contains scripts to make portable binary Linux builds for the firebird and sqlite3 dbi
* build/make_dbi_firebird.sh
* build/make_dbi_sqlite3.sh

How to contact me
-----------------

Peter.DeRijk@uantwerpen.be

Peter De Rijk
VIB - UAntwerp Center for Molecular Neurology
Universiteitsplein 1
B-2610 Antwerp

Legalities
----------

dbi is Copyright Peter De Rijk, University of Antwerp (UIA), 2000
The following terms apply to all files associated with the software unless 
explicitly disclaimed in individual files.

The author hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors
and need not follow the licensing terms described here, provided that
the new terms are clearly indicated on the first page of each file where
they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.
