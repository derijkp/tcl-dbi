
AC_DEFUN(SC_POSTGRESQL, [
	SC_POSTGRESQL_INCLUDE
	SC_POSTGRESQL_LIB
])
#------------------------------------------------------------------------
# SC_POSTGRESQL_INCLUDE --
#
#	Locate the installed public postgresql header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-postgresqlinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		POSTGRESQL_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_POSTGRESQL_INCLUDE, [
    AC_MSG_CHECKING(for postgresql header files)
    AC_ARG_WITH(postgresqlinclude, [ --with-postgresqlinclude      directory containing the postgresql header files.], with_postgresqlinclude=${withval})
    if test x"${with_postgresqlinclude}" != x ; then
	if test -f "${with_postgresqlinclude}/libpq-fe.h" ; then
	    ac_cv_c_postgresqlinclude=${with_postgresqlinclude}
	else
	    AC_MSG_ERROR([${with_postgresqlinclude} directory does not contain postgresql public header file postgresql.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_postgresqlinclude, [
	    # Use the value from --with-postgresqlinclude, if it was given
	    if test x"${with_postgresqlinclude}" != x ; then
		ac_cv_c_postgresqlinclude=${with_postgresqlinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/usr/include/pgsql" "/usr/local/include/pgsql"; do
		    if test -f "$i/libpq-fe.h" ; then
				ac_cv_c_postgresqlinclude=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_postgresqlinclude}" = x ; then
	AC_MSG_ERROR(libpq-fe.h not found.  Please specify its location with --with-postgresqlinclude)
    else
	AC_MSG_RESULT(${ac_cv_c_postgresqlinclude})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_postgresqlinclude}"`
    POSTGRESQL_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(POSTGRESQL_INCLUDE)
])
#------------------------------------------------------------------------
# SC_POSTGRESQL_LIB --
#
#	Locate the installed public postgresql library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-postgresql switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		POSTGRESQL_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_POSTGRESQL_LIB, [
    AC_MSG_CHECKING(for postgresql library files)
    AC_ARG_WITH(postgresql, [ --with-postgresql      directory containing the postgresql library files.], with_postgresql=${withval})
    if test x"${with_postgresql}" != x ; then
	if test -f "${with_postgresql}/libpq.so" ; then
	    ac_cv_c_postgresql=${with_postgresql}
	elif test -f "${with_postgresql}/libpq.a" ; then
	    ac_cv_c_postgresql=${with_postgresql}
	elif test -f "${with_postgresql}/pq.lib" ; then
	    ac_cv_c_postgresql=${with_postgresql}
	else
	    AC_MSG_ERROR([${with_postgresql} directory does not contain postgresql public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_postgresql, [
	    # Use the value from --with-postgresql, if it was given
	    if test x"${with_postgresql}" != x ; then
		ac_cv_c_postgresql=${with_postgresql}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "" ; do
		    if test -f "$i/libpq.so" ; then
				ac_cv_c_postgresql=$i
				break
		    elif test -f "$i/libpq.a" ; then
				ac_cv_c_postgresql=$i
				break
		    elif test -f "$i/pq.lib" ; then
				ac_cv_c_postgresql=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_postgresql}" = x ; then
			AC_MSG_ERROR(postgresql.lib not found.  Please specify its location with --with-postgresql)
		    else
			AC_MSG_RESULT(${ac_cv_c_postgresql})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    POSTGRESQL_LIB=\"`${CYGPATH} "${ac_cv_c_postgresql}/pq.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_postgresql}" = x ; then
			AC_MSG_ERROR(libpq.so or libpq.a not found.  Please specify its location with --with-postgresql)
		    else
			AC_MSG_RESULT(${ac_cv_c_postgresql})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_postgresql}"`
		    if test "$dostatic" == "no" ; then
			    POSTGRESQL_LIB="-L\"${LIB_DIR_NATIVE}\" -lpq"
		    else
			    POSTGRESQL_LIB="\"${LIB_DIR_NATIVE}/libpq.a\""
		    fi
		;;
	esac
    AC_SUBST(POSTGRESQL_LIB)
])

