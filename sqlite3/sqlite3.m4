
AC_DEFUN(SC_SQLITE3, [
	SC_SQLITE3_INCLUDE
	SC_SQLITE3_LIB
])
#------------------------------------------------------------------------
# SC_SQLITE3_INCLUDE --
#
#	Locate the installed public sqlite3 header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-sqlite3include switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		SQLITE3_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_SQLITE3_INCLUDE, [
    AC_MSG_CHECKING(for sqlite3 header files)
    AC_ARG_WITH(sqlite3include, [ --with-sqlite3include      directory containing the sqlite3 header files.], with_sqlite3include=${withval})
    if test x"${with_sqlite3include}" != x ; then
	if test -f "${with_sqlite3include}/sqlite3.h" ; then
	    ac_cv_c_sqlite3include=${with_sqlite3include}
	else
	    AC_MSG_ERROR([${with_sqlite3include} directory does not contain sqlite3 public header file sqlite3.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_sqlite3include, [
	    # Use the value from --with-sqlite3include, if it was given
	    if test x"${with_sqlite3include}" != x ; then
		ac_cv_c_sqlite3include=${with_sqlite3include}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/usr/local/lib"; do
		    if test -f "$i/sqlite3.h" ; then
				ac_cv_c_sqlite3include=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_sqlite3include}" = x ; then
	AC_MSG_ERROR(sqlite3.h not found.  Please specify its location with --with-sqlite3include)
    else
	AC_MSG_RESULT(${ac_cv_c_sqlite3include})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_sqlite3include}"`
    SQLITE3_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(SQLITE3_INCLUDE)
])
#------------------------------------------------------------------------
# SC_SQLITE3_LIB --
#
#	Locate the installed public sqlite3 library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-sqlite3 switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		SQLITE3_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_SQLITE3_LIB, [
    AC_MSG_CHECKING(for sqlite3 library files)
    AC_ARG_WITH(sqlite3, [ --with-sqlite3      directory containing the sqlite3 library files.], with_sqlite3=${withval})
    if test x"${with_sqlite3}" != x ; then
	if test -f "${with_sqlite3}/libsqlite3.so" ; then
	    ac_cv_c_sqlite3=${with_sqlite3}
	elif test -f "${with_sqlite3}/libsqlite3.a" ; then
	    ac_cv_c_sqlite3=${with_sqlite3}
	elif test -f "${with_sqlite3}/sqlite3.lib" ; then
	    ac_cv_c_sqlite3=${with_sqlite3}
	else
	    AC_MSG_ERROR([${with_sqlite3} directory does not contain sqlite3 public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_sqlite3, [
	    # Use the value from --with-sqlite3, if it was given
	    if test x"${with_sqlite3}" != x ; then
		ac_cv_c_sqlite3=${with_sqlite3}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/usr/local/lib" ; do
		    if test -f "$i/libsqlite3.so" ; then
				ac_cv_c_sqlite3=$i
				break
		    elif test -f "$i/libsqlite3.a" ; then
				ac_cv_c_sqlite3=$i
				break
		    elif test -f "$i/sqlite3.lib" ; then
				ac_cv_c_sqlite3=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_sqlite3}" = x ; then
			AC_MSG_ERROR(sqlite3.lib not found.  Please specify its location with --with-sqlite3)
		    else
			AC_MSG_RESULT(${ac_cv_c_sqlite3})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    SQLITE3_LIB=\"`${CYGPATH} "${ac_cv_c_sqlite3}/sqlite3.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_sqlite3}" = x ; then
			AC_MSG_ERROR(libsqlite3.so or libsqlite3.a not found.  Please specify its location with --with-sqlite3)
		    else
			AC_MSG_RESULT(${ac_cv_c_sqlite3})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_sqlite3}"`
		    if test "$dostatic" == "no" ; then
			    SQLITE3_LIB="-L\"${LIB_DIR_NATIVE}\" -lsqlite3"
		    else
			    SQLITE3_LIB="\"${LIB_DIR_NATIVE}/libsqlite3.a\""
		    fi
		;;
	esac
    AC_SUBST(SQLITE3_LIB)
])

