
AC_DEFUN(SC_IBASE, [
	SC_IBASE_INCLUDE
	SC_IBASE_LIB
])
#------------------------------------------------------------------------
# SC_IBASE_INCLUDE --
#
#	Locate the installed public ibase header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-ibaseinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		IBASE_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_IBASE_INCLUDE, [
    AC_MSG_CHECKING(for ibase header files)
    AC_ARG_WITH(ibaseinclude, [ --with-ibaseinclude      directory containing the ibase header files.], with_ibaseinclude=${withval})
    if test x"${with_ibaseinclude}" != x ; then
	if test -f "${with_ibaseinclude}/ibase.h" ; then
	    ac_cv_c_ibaseinclude=${with_ibaseinclude}
	else
	    AC_MSG_ERROR([${with_ibaseinclude} directory does not contain ibase public header file ibase.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_ibaseinclude, [
	    # Use the value from --with-ibaseinclude, if it was given
	    if test x"${with_ibaseinclude}" != x ; then
		ac_cv_c_ibaseinclude=${with_ibaseinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/Program Files/Firebird/include" "/Program Files/borland/interBase/Include" "/Program Files/Borland/InterBase/SDK/include"; do
		    if test -f "$i/ibase.h" ; then
				ac_cv_c_ibaseinclude=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_ibaseinclude}" = x ; then
	AC_MSG_ERROR(ibase.h not found.  Please specify its location with --with-ibaseinclude)
    else
	AC_MSG_RESULT(${ac_cv_c_ibaseinclude})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_ibaseinclude}"`
    IBASE_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(IBASE_INCLUDE)
])
#------------------------------------------------------------------------
# SC_IBASE_LIB --
#
#	Locate the installed public ibase library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-ibase switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		IBASE_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_IBASE_LIB, [
    AC_MSG_CHECKING(for ibase library files)
    AC_ARG_WITH(ibase, [ --with-ibase      directory containing the ibase library files.], with_ibase=${withval})
    if test x"${with_ibase}" != x ; then
	if test -f "${with_ibase}/libfbclient.so" ; then
	    ac_cv_c_ibase=${with_ibase}
	elif test -f "${with_ibase}/libfbclient.a" ; then
	    ac_cv_c_ibase=${with_ibase}
	elif test -f "${with_ibase}/fbclient.lib" ; then
	    ac_cv_c_ibase=${with_ibase}
	else
	    AC_MSG_ERROR([${with_ibase} directory does not contain ibase public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_ibase, [
	    # Use the value from --with-ibase, if it was given
	    if test x"${with_ibase}" != x ; then
		ac_cv_c_ibase=${with_ibase}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/Program Files/Firebird/lib" "/Program Files/borland/interBase/Lib" "/Program Files/Borland/InterBase/SDK/lib_ms" ; do
		    if test -f "$i/libfbclient.so" ; then
				ac_cv_c_ibase=$i
				break
		    elif test -f "$i/libfbclient.a" ; then
				ac_cv_c_ibase=$i
				break
		    elif test -f "$i/fbclient.lib" ; then
				ac_cv_c_ibase=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_ibase}" = x ; then
			AC_MSG_ERROR(ibase.lib not found.  Please specify its location with --with-ibase)
		    else
			AC_MSG_RESULT(${ac_cv_c_ibase})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    IBASE_LIB=\"`${CYGPATH} "${ac_cv_c_ibase}/fbclient.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_ibase}" = x ; then
			AC_MSG_ERROR(libfbclient.so or libfbclient.a not found.  Please specify its location with --with-ibase)
		    else
			AC_MSG_RESULT(${ac_cv_c_ibase})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_ibase}"`
		
		    IBASE_LIB="-L\"${LIB_DIR_NATIVE}\" -lfbclient"
		;;
	esac
    AC_SUBST(IBASE_LIB)
])

