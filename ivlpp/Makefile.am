# Copyright CERN 2016 Michele Castellana <michele.castellana@cern.ch>
# 
# This source code is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This source code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this code.  If not, see <http://www.gnu.org/licenses/>.

#---------------------------------------------------------------------------

noinst_LTLIBRARIES = libivlpp.la

libivlpp_la_CPPFLAGS = \
	-I$(top_srcdir)/ivlpp \
	-I$(top_srcdir)

libivlpp_la_SOURCES = \
	pplexor.c

pplexor.c: $(srcdir)/lexor.lex
	$(LEX) -t $< > $@
