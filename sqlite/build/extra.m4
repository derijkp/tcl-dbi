#------------------------------------------------------------------------
# SC_ENABLE_STUBS --
#
#	Allows the building of stubs libraries
#
# Arguments:
#	none
#	
# Results:
#
#	Adds the following arguments to configure:
#		--enable-stubs=yes|no
#
#	Defines the following vars:
#		USE_TCL_STUBS
#------------------------------------------------------------------------

AC_DEFUN(SC_ENABLE_STUBS, [
    AC_MSG_CHECKING([how to build libraries])
    AC_ARG_ENABLE(stubs,
	[  --enable-stubs         build and link with stubs [--enable-stubs]],
	[tcl_ok=$enableval], [tcl_ok=yes])

    if test "${enable_stubs+set}" = set; then
	enableval="$enable_stubs"
	tcl_ok=$enableval
    else
	tcl_ok=yes
    fi

    if test "$tcl_ok" = "yes" ; then
	AC_MSG_RESULT([stubs])
	AC_DEFINE(USE_TCL_STUBS)
	USE_STUBS=1
    else
	AC_MSG_RESULT([no stubs])
	USE_STUBS=0
    fi
])

#------------------------------------------------------------------------
# SC_TCLLIBDIR --
#
#	Where to place the tcl library files
#
# Arguments:
#	None.
#
# Requires:
#	CYGPATH must be set
#
# Results:
#
#	Adds a --with-tcllibdir switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		tcllibdir
#------------------------------------------------------------------------

AC_DEFUN(SC_TCLLIBDIR, [
	tcllibdir='${prefix}/lib'
	AC_ARG_WITH(tcllibdir, [ --with-tcllibdir      root directory to place architecture-independend library files.], with_tcllibdir=${withval})
	if test x"${with_tcllibdir}" != x ; then
		# Use the value from --with-tcllibdir, if it was given
		tcllibdir=${with_tcllibdir}
	fi
	AC_SUBST(tcllibdir)
])

