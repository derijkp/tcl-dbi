# odbc.m4 --
#

#------------------------------------------------------------------------
# SC_ODBC --
#
#	Locate the installed public ODBC files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-odbcinclude switch to configure.
#	Adds a --with-odbclib switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		ODBC_INCLUDE
#		ODBC_LIB
#		ODBC_LIB_DIR
#------------------------------------------------------------------------
AC_DEFUN(SC_ODBC, [
  case "`uname -s`" in
	*win32* | *WIN32* | *CYGWIN_NT*|*CYGWIN_98*|*CYGWIN_95*)
	  ODBC_INCLUDE=""
	  ODBC_LIB="odbc32.lib odbccp32.lib"
	  AC_SUBST(ODBC_INCLUDE)
	  AC_SUBST(ODBC_LIB)
	;;
	*)
	  SC_ODBC_INCLUDE
	  SC_ODBC_LIB
	;;
  esac
])


#------------------------------------------------------------------------
# SC_ODBC_INCLUDE --
#
#	Locate the installed public ODBC header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-odbcinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		ODBC_INCLUDE
#------------------------------------------------------------------------

AC_DEFUN(SC_ODBC_INCLUDE, [
	AC_MSG_CHECKING(for ODBC header files)

	AC_ARG_WITH(odbcinclude, [ --with-odbcinclude   directory containing the ODBC header files.], with_odbcinclude=${withval})

	if test x"${with_odbcinclude}" != x ; then
	if test -f "${with_odbcinclude}/sql.h" ; then
		ac_cv_c_odbcinclude=${with_odbcinclude}
	else
		AC_MSG_ERROR([${with_odbcinclude} directory does not contain odbc public header file tcl.h])
	fi
	else
	AC_CACHE_VAL(ac_cv_c_odbcinclude, [
		# Use the value from --with-odbcinclude, if it was given

		if test x"${with_odbcinclude}" != x ; then
		ac_cv_c_odbcinclude=${with_odbcinclude}
		else
		# Check in the includedir, if --prefix was specified

		eval "temp_includedir=${includedir}"
		for i in \
			`ls -d ${temp_includedir} 2>/dev/null` \
			/usr/local/include /usr/include ; do
			if test -f "$i/sql.h" ; then
				ac_cv_c_odbcinclude=$i
				break
			elif test -f "$i/odbc/sql.h" ; then
				ac_cv_c_odbcinclude=$i/odbc
				break
			fi
		done
		fi
	])
	fi

	# Print a message based on how we determined the include path

	if test x"${ac_cv_c_odbcinclude}" = x ; then
	AC_MSG_ERROR(sql.h not found.  Please specify its location with --with-odbcinclude)
	else
	AC_MSG_RESULT(${ac_cv_c_odbcinclude})
	fi

	# Convert to a native path and substitute into the output files.

	INCLUDE_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_odbcinclude}`

	ODBC_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"

	AC_SUBST(ODBC_INCLUDE)
])

#------------------------------------------------------------------------
# SC_ODBC_LIB --
#
#	Locate the installed public odbc library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-odbclib switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		ODBC_LIB
#------------------------------------------------------------------------

AC_DEFUN(SC_ODBC_LIB, [
	AC_MSG_CHECKING(for odbc library files)

	AC_ARG_WITH(odbclib, [ --with-odbclib	  directory containing the odbc library files.], with_odbclib=${withval})

	if test x"${with_odbclib}" != x ; then
	if test -f "${with_odbclib}/libodbc.so" ; then
		ac_cv_c_odbclib=${with_odbclib}
	elif test -f "${with_odbclib}/libodbc.a" ; then
		ac_cv_c_odbclib=${with_odbclib}
	else
		AC_MSG_ERROR([${with_odbclib} directory does not contain odbc public header file tcl.h])
	fi
	else
	AC_CACHE_VAL(ac_cv_c_odbclib, [
		# Use the value from --with-odbclib, if it was given

		if test x"${with_odbclib}" != x ; then
		ac_cv_c_odbclib=${with_odbclib}
		else
		# Check in the libdir, if --prefix was specified

		eval "temp_libdir=${libdir}"
		for i in \
			`ls -d ${temp_libdir} 2>/dev/null` \
			/usr/local/lib /usr/lib ; do
			if test -f "$i/libodbc.so" ; then
				ac_cv_c_odbclib=$i
				break
			elif test -f "$i/libodbc.a" ; then
				ac_cv_c_odbclib=$i
				break
			fi
		done
		fi
	])
	fi

	# Print a message based on how we determined the library path

	if test x"${ac_cv_c_odbclib}" = x ; then
	AC_MSG_ERROR(libodbc.so or libodbc.a not found.  Please specify its location with --with-odbclib)
	else
	AC_MSG_RESULT(${ac_cv_c_odbclib})
	fi

	# Convert to a native path and substitute into the output files.

	LIB_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_odbclib}`

	ODBC_LIB="-L\"${LIB_DIR_NATIVE}\" -lodbc"

	AC_SUBST(ODBC_LIB)
])
