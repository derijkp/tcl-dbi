
AC_DEFUN(SC_INTERBASE, [
	SC_INTERBASE_INCLUDE
	SC_INTERBASE_LIB
])
#------------------------------------------------------------------------
# SC_INTERBASE_INCLUDE --
#
#	Locate the installed public interbase header files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-interbaseinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		INTERBASE_INCLUDE
#------------------------------------------------------------------------
AC_DEFUN(SC_INTERBASE_INCLUDE, [
    AC_MSG_CHECKING(for interbase header files)
    AC_ARG_WITH(interbaseinclude, [ --with-interbaseinclude      directory containing the interbase header files.], with_interbaseinclude=${withval})
    if test x"${with_interbaseinclude}" != x ; then
	if test -f "${with_interbaseinclude}/ibase.h" ; then
	    ac_cv_c_interbaseinclude=${with_interbaseinclude}
	else
	    AC_MSG_ERROR([${with_interbaseinclude} directory does not contain interbase public header file interbase.h])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_interbaseinclude, [
	    # Use the value from --with-interbaseinclude, if it was given
	    if test x"${with_interbaseinclude}" != x ; then
		ac_cv_c_interbaseinclude=${with_interbaseinclude}
	    else
		# Check in the includedir, if --prefix was specified
		eval "temp_includedir=${includedir}"
		for i in  `ls -d ${temp_includedir} 2>/dev/null`  /usr/local/include /usr/include "/Program Files/Firebird/include" "/Program Files/borland/interBase/Include" "/Program Files/Borland/InterBase/SDK/include"; do
		    if test -f "$i/ibase.h" ; then
				ac_cv_c_interbaseinclude=$i
				break
		    fi
		done
	    fi
	])
    fi
    # Print a message based on how we determined the include path
    if test x"${ac_cv_c_interbaseinclude}" = x ; then
	AC_MSG_ERROR(ibase.h not found.  Please specify its location with --with-interbaseinclude)
    else
	AC_MSG_RESULT(${ac_cv_c_interbaseinclude})
    fi
    # Convert to a native path and substitute into the output files.
    INCLUDE_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_interbaseinclude}"`
    INTERBASE_INCLUDE=-I\"${INCLUDE_DIR_NATIVE}\"
    AC_SUBST(INTERBASE_INCLUDE)
])
#------------------------------------------------------------------------
# SC_INTERBASE_LIB --
#
#	Locate the installed public interbase library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-interbase switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		INTERBASE_LIB
#------------------------------------------------------------------------
AC_DEFUN(SC_INTERBASE_LIB, [
    AC_MSG_CHECKING(for interbase library files)
    AC_ARG_WITH(interbase, [ --with-interbase      directory containing the interbase library files.], with_interbase=${withval})
    if test x"${with_interbase}" != x ; then
	if test -f "${with_interbase}/libfbembed.so" ; then
	    ac_cv_c_interbase=${with_interbase}
	elif test -f "${with_interbase}/libfbembed.a" ; then
	    ac_cv_c_interbase=${with_interbase}
	elif test -f "${with_interbase}/fbclient.lib" ; then
	    ac_cv_c_interbase=${with_interbase}
	else
	    AC_MSG_ERROR([${with_interbase} directory does not contain interbase public library file $libzfile])
	fi
    else
	AC_CACHE_VAL(ac_cv_c_interbase, [
	    # Use the value from --with-interbase, if it was given
	    if test x"${with_interbase}" != x ; then
		ac_cv_c_interbase=${with_interbase}
	    else
		# Check in the libdir, if --prefix was specified
		eval "temp_libdir=${libdir}"
		for i in  `ls -d ${temp_libdir} 2>/dev/null`  /usr/local/lib /usr/lib "/Program Files/Firebird/lib" "/Program Files/borland/interBase/Lib" "/Program Files/Borland/InterBase/SDK/lib_ms" ; do
		    if test -f "$i/libfbembed.so" ; then
				ac_cv_c_interbase=$i
				break
		    elif test -f "$i/libfbembed.a" ; then
				ac_cv_c_interbase=$i
				break
		    elif test -f "$i/fbclient.lib" ; then
				ac_cv_c_interbase=$i
				break
		    fi
		done
	    fi
	])
    fi
	case "`uname -s`" in
		*win32* | *WIN32* | *CYGWIN_NT* |*CYGWIN_98*|*CYGWIN_95*)
		    if test x"${ac_cv_c_interbase}" = x ; then
			AC_MSG_ERROR(interbase.lib not found.  Please specify its location with --with-interbase)
		    else
			AC_MSG_RESULT(${ac_cv_c_interbase})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    INTERBASE_LIB=\"`${CYGPATH} "${ac_cv_c_interbase}/fbclient.lib"`\"
		;;
		*)
		    if test x"${ac_cv_c_interbase}" = x ; then
			AC_MSG_ERROR(libfbembed.so or libfbembed.a not found.  Please specify its location with --with-interbase)
		    else
			AC_MSG_RESULT(${ac_cv_c_interbase})
		    fi
		
		    # Convert to a native path and substitute into the output files.
		
		    LIB_DIR_NATIVE=`${CYGPATH} "${ac_cv_c_interbase}"`
		
		    INTERBASE_LIB="-L\"${LIB_DIR_NATIVE}\" -lfbembed"
		;;
	esac
    AC_SUBST(INTERBASE_LIB)
])

