include $(top_srcdir)/Makefile.tool.am

EXTRA_DIST = docs/fi-manual.xml

pkginclude_HEADERS = fi_client.h

#----------------------------------------------------------------------------
# fitin-<platform>
#----------------------------------------------------------------------------

noinst_PROGRAMS  = fitin-@VGCONF_ARCH_PRI@-@VGCONF_OS@
if VGCONF_HAVE_PLATFORM_SEC
noinst_PROGRAMS += fitin-@VGCONF_ARCH_SEC@-@VGCONF_OS@
endif

FITIN_SOURCES_COMMON = fi_main.c fi_reg.c lua_vg.c lua/src/llex.c lua/src/lapi.c lua/src/lundump.c lua/src/ltm.c lua/src/lstate.c lua/src/lopcodes.c lua/src/lauxlib.c lua/src/loadlib.c lua/src/lbitlib.c lua/src/ldo.c lua/src/lcode.c lua/src/ldebug.c lua/src/lmathlib.c lua/src/lvm.c lua/src/lstring.c lua/src/lmem.c lua/src/lctype.c lua/src/ldblib.c lua/src/liolib.c lua/src/lfunc.c lua/src/ldump.c lua/src/lzio.c lua/src/loslib.c lua/src/ltable.c lua/src/lobject.c lua/src/linit.c lua/src/lbaselib.c lua/src/lparser.c lua/src/ltablib.c lua/src/lgc.c lua/src/lcorolib.c lua/src/lstrlib.c lua/src/lfs.c

fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_SOURCES      = \
	$(FITIN_SOURCES_COMMON)
fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS       = \
	$(AM_CFLAGS_@VGCONF_PLATFORM_PRI_CAPS@) -DFITIN_WITH_LUA -Ilua/src
fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_PRI_CAPS@)
fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_PRI_CAPS@)
fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_PRI_CAPS@) 
fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_PRI@ \
	$(LINK) \
	$(fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_CFLAGS) \
	$(fitin_@VGCONF_ARCH_PRI@_@VGCONF_OS@_LDFLAGS)

if VGCONF_HAVE_PLATFORM_SEC
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_SOURCES      = \
	$(FITIN_SOURCES_COMMON)
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CPPFLAGS     = \
	$(AM_CPPFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS       = \
	$(AM_CFLAGS_@VGCONF_PLATFORM_SEC_CAPS@) -DFITIN_WITH_LUA -Ilua/src
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_DEPENDENCIES = \
	$(TOOL_DEPENDENCIES_@VGCONF_PLATFORM_SEC_CAPS@)
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDADD        = \
	$(TOOL_LDADD_@VGCONF_PLATFORM_SEC_CAPS@)
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS      = \
	$(TOOL_LDFLAGS_@VGCONF_PLATFORM_SEC_CAPS@)
fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LINK = \
	$(top_builddir)/coregrind/link_tool_exe_@VGCONF_OS@ \
	@VALT_LOAD_ADDRESS_SEC@ \
	$(LINK) \
	$(fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_CFLAGS) \
	$(fitin_@VGCONF_ARCH_SEC@_@VGCONF_OS@_LDFLAGS)
endif

