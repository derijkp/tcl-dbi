
AC_DEFUN(SC_FIREBIRD, [
	SC_FIREBIRD_INCLUDE
	SC_FIREBIRD_LIB
])
#------------------------------------------------------------------------
# SC_FIREBIRD_INCLUDE --
#
#	Locate the installed public firebird header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-firebirdinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		FIREBIRD_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_FIREBIRD_INCLUDE, [
    AC_MSG_CHECKING(for firebird header files)
    AC_ARG_WITH(firebirdinclude, [ --with-firebirdinclude      directory containing the firebird header files.], with_firebirdinclude=${withval})
    if test x"${with_firebirdinclude}" != x ; then
	if test -f "${with_firebirdinclude}/ibase.h" ; then
	    ac_cv_c_firebirdinclude=${with_firebirdinclude}
	else
	    AC_MSG_ERROR([${with_firebirdinclude} directory does not contain firebird public header file firebird.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_firebirdinclude, [
	    # Use the value from --with-firebirdinclude, if it was given
	    if test x"${with_firebirdinclude}" != x ; then
		ac_cv_c_firebirdinclude=${with_firebirdinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/Program Files/Firebird/include" "/Program Files/borland/interBase/Include" "/Program Files/Borland/InterBase/SDK/include"; do
		    if test -f "$i/ibase.h" ; then
				ac_cv_c_firebirdinclude=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_firebirdinclude}" = x ; then
	AC_MSG_ERROR(ibase.h not found.  Please specify its location with --with-firebirdinclude)
    else
	AC_MSG_RESULT(${ac_cv_c_firebirdinclude})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_firebirdinclude}"`
    FIREBIRD_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(FIREBIRD_INCLUDE)
])
#------------------------------------------------------------------------
# SC_FIREBIRD_LIB --
#
#	Locate the installed public firebird library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-firebird switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		FIREBIRD_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_FIREBIRD_LIB, [
    AC_MSG_CHECKING(for firebird library files)
    AC_ARG_WITH(firebird, [ --with-firebird      directory containing the firebird library files.], with_firebird=${withval})
    if test x"${with_firebird}" != x ; then
	if test -f "${with_firebird}/libgds.so" ; then
	    ac_cv_c_firebird=${with_firebird}
	elif test -f "${with_firebird}/libgds.a" ; then
	    ac_cv_c_firebird=${with_firebird}
	elif test -f "${with_firebird}/gds32_ms.lib" ; then
	    ac_cv_c_firebird=${with_firebird}
	else
	    AC_MSG_ERROR([${with_firebird} directory does not contain firebird public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_firebird, [
	    # Use the value from --with-firebird, if it was given
	    if test x"${with_firebird}" != x ; then
		ac_cv_c_firebird=${with_firebird}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/Program Files/Firebird/lib" "/Program Files/borland/interBase/Lib" "/Program Files/Borland/InterBase/SDK/lib_ms" ; do
		    if test -f "$i/libgds.so" ; then
				ac_cv_c_firebird=$i
				break
		    elif test -f "$i/libgds.a" ; then
				ac_cv_c_firebird=$i
				break
		    elif test -f "$i/gds32_ms.lib" ; then
				ac_cv_c_firebird=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_firebird}" = x ; then
			AC_MSG_ERROR(firebird.lib not found.  Please specify its location with --with-firebird)
		    else
			AC_MSG_RESULT(${ac_cv_c_firebird})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    FIREBIRD_LIB=\"`${CYGPATH} "${ac_cv_c_firebird}/gds32_ms.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_firebird}" = x ; then
			AC_MSG_ERROR(libgds.so or libgds.a not found.  Please specify its location with --with-firebird)
		    else
			AC_MSG_RESULT(${ac_cv_c_firebird})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_firebird}"`
		    if test "$dostatic" == "no" ; then
			    FIREBIRD_LIB="-L\"${LIB_DIR_NATIVE}\" -lgds"
		    else
			    FIREBIRD_LIB="\"${LIB_DIR_NATIVE}/libgds.a\""
		    fi
		;;
	esac
    AC_SUBST(FIREBIRD_LIB)
])

