
AC_DEFUN(SC_ODBC, [
	SC_ODBC_INCLUDE
	SC_ODBC_LIB
])
#------------------------------------------------------------------------
# SC_ODBC_INCLUDE --
#
#	Locate the installed public odbc header files
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
    AC_MSG_CHECKING(for odbc header files)
    AC_ARG_WITH(odbcinclude, [ --with-odbcinclude      directory containing the odbc header files.], with_odbcinclude=${withval})
    if test x"${with_odbcinclude}" != x ; then
	if test -f "${with_odbcinclude}/sql.h" ; then
	    ac_cv_c_odbcinclude=${with_odbcinclude}
	else
	    AC_MSG_ERROR([${with_odbcinclude} directory does not contain odbc public header file odbc.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_odbcinclude, [
	    # Use the value from --with-odbcinclude, if it was given
	    if test x"${with_odbcinclude}" != x ; then
		ac_cv_c_odbcinclude=${with_odbcinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/c/Program Files/Microsoft Data Access SDK/inc"; do
		    if test -f "$i/sql.h" ; then
				ac_cv_c_odbcinclude=$i
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
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_odbcinclude}"`
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
#	Adds a --with-odbc switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		ODBC_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_ODBC_LIB, [
    AC_MSG_CHECKING(for odbc library files)
    AC_ARG_WITH(odbc, [ --with-odbc      directory containing the odbc library files.], with_odbc=${withval})
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*|*MINGW*)
		    if test x"odbc32" == x ; then
			ODBC_LIB=""
		    else
			    if test x"${with_odbc}" != x ; then
				if test -f "${with_odbc}/odbc32.lib" ; then
				    ac_cv_c_odbc=${with_odbc}
				else
				    AC_MSG_ERROR([${with_odbc} directory does not contain odbc public library file $libzfile])
				fi
			    else
				AC_CACHE_VAL(ac_cv_c_odbc, [
				    # Use the value from --with-odbc, if it was given
				    if test x"${with_odbc}" != x ; then
					ac_cv_c_odbc=${with_odbc}
				    else
					# Check in the libdir, if --prefix was specified
					eval "temp_libdir=${libdir}"
					for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/c/Program Files/Microsoft Data Access SDK/lib/x86" ; do
					    if test -f "$i/odbc32.lib" ; then
							ac_cv_c_odbc=$i
							break
					    fi
					done
				    fi
				])
			    fi
			    if test x"${ac_cv_c_odbc}" = x ; then
				AC_MSG_ERROR(odbc.lib not found.  Please specify its location with --with-odbc)
			    else
				AC_MSG_RESULT(${ac_cv_c_odbc})
			    fi
			
			    # Convert to a native path and substitute into the output files.
			
			    ODBC_LIB=\"`${CYGPATH} "${ac_cv_c_odbc}/odbc32.lib"`\"
		    fi
		;;
		*)
		    if test x"odbc" == x ; then
			ODBC_LIB=""
		    else
			    if test x"${with_odbc}" != x ; then
				if test -f "${with_odbc}/libodbc.so" ; then
				    ac_cv_c_odbc=${with_odbc}
				elif test -f "${with_odbc}/libodbc.a" ; then
				    ac_cv_c_odbc=${with_odbc}
				else
				    AC_MSG_ERROR([${with_odbc} directory does not contain odbc public library file $libzfile])
				fi
			    else
				AC_CACHE_VAL(ac_cv_c_odbc, [
				    # Use the value from --with-odbc, if it was given
				    if test x"${with_odbc}" != x ; then
					ac_cv_c_odbc=${with_odbc}
				    else
					# Check in the libdir, if --prefix was specified
					eval "temp_libdir=${libdir}"
					for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/c/Program Files/Microsoft Data Access SDK/lib/x86" ; do
					    if test -f "$i/libodbc.so" ; then
							ac_cv_c_odbc=$i
							break
					    elif test -f "$i/libodbc.a" ; then
							ac_cv_c_odbc=$i
							break
					    fi
					done
				    fi
				])
			    fi
			    if test x"${ac_cv_c_odbc}" = x ; then
				AC_MSG_ERROR(libodbc.so or libodbc.a not found.  Please specify its location with --with-odbc)
			    else
				AC_MSG_RESULT(${ac_cv_c_odbc})
			    fi
			
			    # Convert to a native path and substitute into the output files.
			
			    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_odbc}"`
			
			    ODBC_LIB="-L\"${LIB_DIR_NATIVE}\" -lodbc"
		    fi
		;;
	esac
    AC_SUBST(ODBC_LIB)
])

