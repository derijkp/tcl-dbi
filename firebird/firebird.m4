
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
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/Program Files/Firebird/include"; do
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
	if test -f "${with_firebird}/libfbclient.so" ; then
	    ac_cv_c_firebird=${with_firebird}
	elif test -f "${with_firebird}/libfbclient.a" ; then
	    ac_cv_c_firebird=${with_firebird}
	elif test -f "${with_firebird}/fbclient_ms.lib" ; then
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
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/Program Files/Firebird/lib" ; do
		    if test -f "$i/libfbclient.so" ; then
				ac_cv_c_firebird=$i
				break
		    elif test -f "$i/libfbclient.a" ; then
				ac_cv_c_firebird=$i
				break
		    elif test -f "$i/fbclient_ms.lib" ; then
				ac_cv_c_firebird=$i
				break
		    fi
		done
	    fi
	])
    fi
    AC_ARG_ENABLE(static, [  --enable-static         link firebird library statically [--disable-static]],[tcl_ok=$enableval], [tcl_ok=no])
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_firebird}" = x ; then
			AC_MSG_ERROR(firebird.lib not found.  Please specify its location with --with-firebird)
		    else
			AC_MSG_RESULT(${ac_cv_c_firebird})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    FIREBIRD_LIB=\"`${CYGPATH} "${ac_cv_c_firebird}/fbclient_ms.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_firebird}" = x ; then
			AC_MSG_ERROR(libfbclient.so or libfbclient.a not found.  Please specify its location with --with-firebird)
		    else
			AC_MSG_RESULT(${ac_cv_c_firebird})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_firebird}"`
		
		    if test "$tcl_ok" = "no"; then
			    FIREBIRD_LIB="-L\"${LIB_DIR_NATIVE}\" -lfbclient"
		    else
			    FIREBIRD_LIB=" ${LIB_DIR_NATIVE}/libfbclient.a "
		    fi
		;;
	esac
    AC_SUBST(FIREBIRD_LIB)
])
