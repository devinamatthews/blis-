bin_PROGRAMS = bin/profile_knl
bin_profile_knl_SOURCES = profile/profile_knl.cxx
	
VPATH += $(srcdir)

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -I$(srcdir)/include -Iinclude @memkind_INCLUDES@ @libhugetlbfs_INCLUDES@ @blis_INCLUDES@
AM_LDFLAGS = -pthread
bin_profile_knl_LDADD = @memkind_LIBS@ @libhugetlbfs_LIBS@ @blis_LIBS@
