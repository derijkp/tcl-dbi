# mysql_config --libmysqld-libs
rm -f libdbi_mysql-embedded1.0.0.so
gcc -pipe -shared -o libdbi_embmysql1.0.0.so mysql.o -L"/usr/lib"  -L/home/peter/tcl/deftcl/lib -ltclstub8.4  -L"/usr/lib" -L/usr/lib/mysql -lmysqld -lpthread -lcrypt -lnsl -lm -lpthread -lrt -lz -lssl
cp libdbi_embmysql1.0.0.so ~/bin/tcl/dbi_embmysql1.0.0/Linux-i686
