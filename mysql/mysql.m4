
AC_DEFUN(SC_MYSQL, [
	SC_MYSQL_INCLUDE
	SC_MYSQL_LIB
])
#------------------------------------------------------------------------
# SC_MYSQL_INCLUDE --
#
#	Locate the installed public mysql header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-mysqlinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		MYSQL_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_MYSQL_INCLUDE, [
    AC_MSG_CHECKING(for mysql header files)
    AC_ARG_WITH(mysqlinclude, [ --with-mysqlinclude      directory containing the mysql header files.], with_mysqlinclude=${withval})
    if test x"${with_mysqlinclude}" != x ; then
	if test -f "${with_mysqlinclude}/mysql.h" ; then
	    ac_cv_c_mysqlinclude=${with_mysqlinclude}
	else
	    AC_MSG_ERROR([${with_mysqlinclude} directory does not contain mysql public header file mysql.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_mysqlinclude, [
	    # Use the value from --with-mysqlinclude, if it was given
	    if test x"${with_mysqlinclude}" != x ; then
		ac_cv_c_mysqlinclude=${with_mysqlinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/usr/include/mysql" "/usr/local/include/mysql"; do
		    if test -f "$i/mysql.h" ; then
				ac_cv_c_mysqlinclude=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_mysqlinclude}" = x ; then
	AC_MSG_ERROR(mysql.h not found.  Please specify its location with --with-mysqlinclude)
    else
	AC_MSG_RESULT(${ac_cv_c_mysqlinclude})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_mysqlinclude}"`
    MYSQL_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(MYSQL_INCLUDE)
])
#------------------------------------------------------------------------
# SC_MYSQL_LIB --
#
#	Locate the installed public mysql library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-mysql switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		MYSQL_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_MYSQL_LIB, [
    AC_MSG_CHECKING(for mysql library files)
    AC_ARG_WITH(mysql, [ --with-mysql      directory containing the mysql library files.], with_mysql=${withval})
    if test x"${with_mysql}" != x ; then
	if test -f "${with_mysql}/libmysqlclient.so" ; then
	    ac_cv_c_mysql=${with_mysql}
	elif test -f "${with_mysql}/libmysqlclient.a" ; then
	    ac_cv_c_mysql=${with_mysql}
	elif test -f "${with_mysql}/mysql.lib" ; then
	    ac_cv_c_mysql=${with_mysql}
	else
	    AC_MSG_ERROR([${with_mysql} directory does not contain mysql public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_mysql, [
	    # Use the value from --with-mysql, if it was given
	    if test x"${with_mysql}" != x ; then
		ac_cv_c_mysql=${with_mysql}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "" ; do
		    if test -f "$i/libmysqlclient.so" ; then
				ac_cv_c_mysql=$i
				break
		    elif test -f "$i/libmysqlclient.a" ; then
				ac_cv_c_mysql=$i
				break
		    elif test -f "$i/mysql.lib" ; then
				ac_cv_c_mysql=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_mysql}" = x ; then
			AC_MSG_ERROR(mysql.lib not found.  Please specify its location with --with-mysql)
		    else
			AC_MSG_RESULT(${ac_cv_c_mysql})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    MYSQL_LIB=\"`${CYGPATH} "${ac_cv_c_mysql}/mysql.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_mysql}" = x ; then
			AC_MSG_ERROR(libmysqlclient.so or libmysqlclient.a not found.  Please specify its location with --with-mysql)
		    else
			AC_MSG_RESULT(${ac_cv_c_mysql})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_mysql}"`
		
		    MYSQL_LIB="-L\"${LIB_DIR_NATIVE}\" -lmysqlclient"
		;;
	esac
    AC_SUBST(MYSQL_LIB)
])

