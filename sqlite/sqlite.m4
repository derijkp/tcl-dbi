
AC_DEFUN(SC_SQLITE, [
	SC_SQLITE_INCLUDE
	SC_SQLITE_LIB
])
#------------------------------------------------------------------------
# SC_SQLITE_INCLUDE --
#
#	Locate the installed public sqlite header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-sqliteinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		SQLITE_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_SQLITE_INCLUDE, [
    AC_MSG_CHECKING(for sqlite header files)
    AC_ARG_WITH(sqliteinclude, [ --with-sqliteinclude      directory containing the sqlite header files.], with_sqliteinclude=${withval})
    if test x"${with_sqliteinclude}" != x ; then
	if test -f "${with_sqliteinclude}/sqlite.h" ; then
	    ac_cv_c_sqliteinclude=${with_sqliteinclude}
	else
	    AC_MSG_ERROR([${with_sqliteinclude} directory does not contain sqlite public header file sqlite.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_sqliteinclude, [
	    # Use the value from --with-sqliteinclude, if it was given
	    if test x"${with_sqliteinclude}" != x ; then
		ac_cv_c_sqliteinclude=${with_sqliteinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include ""; do
		    if test -f "$i/sqlite.h" ; then
				ac_cv_c_sqliteinclude=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_sqliteinclude}" = x ; then
	AC_MSG_ERROR(sqlite.h not found.  Please specify its location with --with-sqliteinclude)
    else
	AC_MSG_RESULT(${ac_cv_c_sqliteinclude})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_sqliteinclude}"`
    SQLITE_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(SQLITE_INCLUDE)
])
#------------------------------------------------------------------------
# SC_SQLITE_LIB --
#
#	Locate the installed public sqlite library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-sqlite switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		SQLITE_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_SQLITE_LIB, [
    AC_MSG_CHECKING(for sqlite library files)
    AC_ARG_WITH(sqlite, [ --with-sqlite      directory containing the sqlite library files.], with_sqlite=${withval})
    if test x"${with_sqlite}" != x ; then
	if test -f "${with_sqlite}/libsqlite.so" ; then
	    ac_cv_c_sqlite=${with_sqlite}
	elif test -f "${with_sqlite}/libsqlite.a" ; then
	    ac_cv_c_sqlite=${with_sqlite}
	elif test -f "${with_sqlite}/sqlite.lib" ; then
	    ac_cv_c_sqlite=${with_sqlite}
	else
	    AC_MSG_ERROR([${with_sqlite} directory does not contain sqlite public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_sqlite, [
	    # Use the value from --with-sqlite, if it was given
	    if test x"${with_sqlite}" != x ; then
		ac_cv_c_sqlite=${with_sqlite}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/usr/local/lib" ; do
		    if test -f "$i/libsqlite.so" ; then
				ac_cv_c_sqlite=$i
				break
		    elif test -f "$i/libsqlite.a" ; then
				ac_cv_c_sqlite=$i
				break
		    elif test -f "$i/sqlite.lib" ; then
				ac_cv_c_sqlite=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_sqlite}" = x ; then
			AC_MSG_ERROR(sqlite.lib not found.  Please specify its location with --with-sqlite)
		    else
			AC_MSG_RESULT(${ac_cv_c_sqlite})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    SQLITE_LIB=\"`${CYGPATH} "${ac_cv_c_sqlite}/sqlite.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_sqlite}" = x ; then
			AC_MSG_ERROR(libsqlite.so or libsqlite.a not found.  Please specify its location with --with-sqlite)
		    else
			AC_MSG_RESULT(${ac_cv_c_sqlite})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_sqlite}"`
		    if test "$dostatic" == "no" ; then
			    SQLITE_LIB="-L\"${LIB_DIR_NATIVE}\" -lsqlite"
		    else
			    SQLITE_LIB="\"${LIB_DIR_NATIVE}/libsqlite.a\""
		    fi
		;;
	esac
    AC_SUBST(SQLITE_LIB)
])

