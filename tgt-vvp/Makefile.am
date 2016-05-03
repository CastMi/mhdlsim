# Copyright (c) 2016 CERN
# @author Michele Castellana <michele.castellana@cern.ch>
#
# This source code is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This source code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this code.  If not, see <http://www.gnu.org/licenses/>.
#
#---------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libvvptarget.la

libvvptarget_la_CPPFLAGS = \
	-I$(top_srcdir) /

libvvptarget_la_SOURCES = \
	draw_class.c \
	draw_enum.c \
	draw_mux.c \
	draw_net_input.c \
	draw_substitute.c \
	draw_switch.c \
	draw_ufunc.c \
	draw_vpi.c \
	eval_bool.c \
	eval_condit.c \
	eval_expr.c \
	eval_object.c \
	eval_real.c \
	eval_string.c \
	eval_vec4.c \
	modpath.c \
	stmt_assign.c \
	vector.c \
	vvp.c \
	vvp_process.c \
	vvp_scope.c

EXTRA_libvvptarget_la_DEPENDENCIES = create_header

create_header:
	if test "x$(GIT)" != "xnone" && test -d "$(top_srcdir)/.git"; then \
	    echo "Using git-describe for VERSION_TAG"; \
	    tmp=`(cd $(srcdir) && $(GIT) describe --always --dirty) \
		   | sed -e 's;\(.*\);#define VERSION_TAG "\1";'`; \
	    echo "$$tmp" | diff - version_tag.h > /dev/null 2>&1 || echo "$$tmp" > version_tag.h || exit 1; \
	elif test -d "$(top_srcdir)/.git"; then \
	    echo "Using $(top_srcdir)/version_tag.h for VERSION_TAG"; \
	    diff $(top_srcdir)/version_tag.h version_tag.h > /dev/null 2>&1 || cp $(top_srcdir)/version_tag.h version_tag.h; \
	else \
	    echo "Using empty VERSION_TAG"; \
	    echo '#define VERSION_TAG ""' > version_tag.h; \
	fi
