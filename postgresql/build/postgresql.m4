# postgresql.m4 --
#

#------------------------------------------------------------------------
# SC_POSTGRESQL --
#
#	Locate the installed public postgresql files
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
#	Adds a --with-postgresqllib switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		POSTGRESQL_INCLUDE
#		POSTGRESQL_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_POSTGRESQL, [
	SC_POSTGRESQL_INCLUDE
	SC_POSTGRESQL_LIB
])

#------------------------------------------------------------------------
# SC_POSTGRESQL_INCLUDE --
#
#	Locate the installed public Postgresql header files
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
    AC_MSG_CHECKING(for Postgresql header files)

    AC_ARG_WITH(postgresqlinclude, [ --with-postgresqlinclude      directory containing the postgresql header files.], with_postgresqlinclude=${withval})

    if test x"${with_postgresqlinclude}" != x ; then
	if test -f "${with_postgresqlinclude}/libpq-fe.h" ; then
	    ac_cv_c_postgresqlinclude=${with_postgresqlinclude}
	elif test -f "${with_postgresqlinclude}/pgsql/libpq-fe.h" ; then
	    ac_cv_c_postgresqlinclude=${with_postgresqlinclude}/pgsql
	else
	    AC_MSG_ERROR([${with_postgresqlinclude} directory does not contain Postgresql public header file tcl.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_postgresqlinclude, [
	    # Use the value from --with-postgresqlinclude, if it was given

	    if test x"${with_postgresqlinclude}" != x ; then
		ac_cv_c_postgresqlinclude=${with_postgresqlinclude}
	    else
		# Check in the includedir, if --prefix was specified

		eval "temp_includedir=${includedir}"
		for i in \
			`ls -d ${temp_includedir} 2>/dev/null` \
			/usr/local/include /usr/include ; do
		    if test -f "$i/libpq-fe.h" ; then
				ac_cv_c_postgresqlinclude=$i
				break
			elif test -f "$i/pgsql/libpq-fe.h" ; then
				ac_cv_c_postgresqlinclude=$i/pgsql
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

    INCLUDE_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_postgresqlinclude}`

    POSTGRESQL_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"

    AC_SUBST(POSTGRESQL_INCLUDE)
])

#------------------------------------------------------------------------
# SC_POSTGRESQL_LIB --
#
#	Locate the installed public Postgresql library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-postgresqllib switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		POSTGRESQL_LIB
#------------------------------------------------------------------------

AC_DEFUN(SC_POSTGRESQL_LIB, [
    AC_MSG_CHECKING(for Postgresql library files)

    AC_ARG_WITH(postgresqllib, [ --with-postgresqllib      directory containing the postgresql library files.], with_postgresqllib=${withval})

    if test x"${with_postgresqllib}" != x ; then
	if test -f "${with_postgresqllib}/libpq.so" ; then
	    ac_cv_c_postgresqllib=${with_postgresqllib}
	elif test -f "${with_postgresqllib}/libpq.a" ; then
	    ac_cv_c_postgresqllib=${with_postgresqllib}
	else
	    AC_MSG_ERROR([${with_postgresqllib} directory does not contain Postgresql public header file tcl.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_postgresqllib, [
	    # Use the value from --with-postgresqllib, if it was given

	    if test x"${with_postgresqllib}" != x ; then
		ac_cv_c_postgresqllib=${with_postgresqllib}
	    else
		# Check in the libdir, if --prefix was specified

		eval "temp_libdir=${libdir}"
		for i in \
			`ls -d ${temp_libdir} 2>/dev/null` \
			/usr/local/lib /usr/lib ; do
		    if test -f "$i/libpq.so" ; then
				ac_cv_c_postgresqllib=$i
				break
		    elif test -f "$i/libpq.a" ; then
				ac_cv_c_postgresqllib=$i
				break
		    fi
		done
	    fi
	])
    fi

    # Print a message based on how we determined the library path

    if test x"${ac_cv_c_postgresqllib}" = x ; then
	AC_MSG_ERROR(libpq.so or libpq.a not found.  Please specify its location with --with-postgresqllib)
    else
	AC_MSG_RESULT(${ac_cv_c_postgresqllib})
    fi

    # Convert to a native path and substitute into the output files.

    LIB_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_postgresqllib}`

    POSTGRESQL_LIB="-L\"${LIB_DIR_NATIVE}\" -lpq"

    AC_SUBST(POSTGRESQL_LIB)
])
