# postgresql.m4 --
#

#------------------------------------------------------------------------
# SC_DBI --
#
#	Locate the installed public dbi files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-dbiinclude switch to configure.
#	Adds a --with-dbilib switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		DBI_INCLUDE
#		DBI_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_DBI, [
	SC_DBI_INCLUDE
])

#------------------------------------------------------------------------
# SC_DBI_INCLUDE --
#
#	Locate the installed public dbi header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-dbiinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		DBI_INCLUDE
#------------------------------------------------------------------------

AC_DEFUN(SC_DBI_INCLUDE, [
	AC_MSG_CHECKING(for dbi header files)

	AC_ARG_WITH(dbiinclude, [ --with-dbiinclude   directory containing the dbi header files.], with_dbiinclude=${withval})

	if test x"${with_dbiinclude}" != x ; then
	if test -f "${with_dbiinclude}/dbi.h" ; then
		ac_cv_c_dbiinclude=${with_dbiinclude}
	else
		AC_MSG_ERROR([${with_dbiinclude} directory does not contain dbi public header file dbi.h])
	fi
	else
	AC_CACHE_VAL(ac_cv_c_dbiinclude, [
		# Use the value from --with-dbiinclude, if it was given

		if test x"${with_dbiinclude}" != x ; then
		ac_cv_c_dbiinclude=${with_dbiinclude}
		else
		# Check in the includedir, if --prefix was specified

		eval "temp_includedir=${includedir}"
		for i in \
			`ls -d ${temp_includedir} 2>/dev/null` \
			../../src ../../../src /usr/local/include /usr/include ; do
			if test -f "$i/dbi.h" ; then
				ac_cv_c_dbiinclude=$i
				break
			fi
		done
		fi
	])
	fi

	# Print a message based on how we determined the include path

	if test x"${ac_cv_c_dbiinclude}" = x ; then
	AC_MSG_ERROR(dbi.h not found.  Please specify its location with --with-dbiinclude)
	else
	AC_MSG_RESULT(${ac_cv_c_dbiinclude})
	fi

	# Convert to a native path and substitute into the output files.

	INCLUDE_DIR_NATIVE=`${CYGPATH} ${ac_cv_c_dbiinclude}`

	DBI_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"

	AC_SUBST(DBI_INCLUDE)
])
