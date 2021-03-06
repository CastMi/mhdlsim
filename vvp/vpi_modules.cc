/*
 * Copyright (c) 2001-2010 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

# include  "config.h"
# include  "vpi_priv.h"
# include  "ivl_dlfcn.h"
# include  "vvp_cleanup.h"
# include  <cstdio>
# include  <cstring>
# include  <sys/types.h>
# include  <sys/stat.h>
# include  "ivl_alloc.h"

static ivl_dll_t*dll_list = 0;
static unsigned dll_list_cnt = 0;

typedef void (*vlog_startup_routines_t)(void);


const char* vpip_module_path[64] = {
#ifdef MODULE_DIR1
      MODULE_DIR1,
#endif
#ifdef MODULE_DIR2
      MODULE_DIR2,
#endif
      0
};

unsigned vpip_module_path_cnt = 0
#ifdef MODULE_DIR1
         + 1
#endif
#ifdef MODULE_DIR2
         + 1
#endif
;

void load_module_delete(void)
{
      for (unsigned idx = 0; idx < dll_list_cnt; idx += 1) {
	    ivl_dlclose(dll_list[idx]);
      }
      free(dll_list);
      dll_list = 0;
      dll_list_cnt = 0;
}

void vpip_load_module(const char*name)
{
      struct stat sb;
      int rc;
      bool export_flag = false;
      char buf[4096];

#ifdef __MINGW32__
      const char sep = '\\';
#else
      const char sep = '/';
#endif

      buf[0] = 0;                     /* terminate the string */
      if (strchr(name, sep)) {
	      /* If the name has at least one directory character in
		 it, then assume it is a complete name, maybe including any
		 possible .vpi suffix. */
	    export_flag = false;
	    rc = stat(name, &sb);

	    if (rc != 0) {            /* did we find a file? */
	          /* no, try with a .vpi suffix too */
		  export_flag = false;
		  sprintf(buf, "%s.vpi", name);
		  rc = stat(buf, &sb);

		    /* Try also with the .vpl suffix. */
		  if (rc != 0) {
			export_flag = true;
			sprintf(buf, "%s.vpl", name);
			rc = stat(buf, &sb);
		  }

		  if (rc != 0) {
			fprintf(stderr, "%s: Unable to find module file `%s' "
				"or `%s.vpi'.\n", name,name,buf);
			return;
		  }
	    } else {
	      strcpy(buf,name);   /* yes copy the name into the buffer */
	    }

      } else {
	    rc = -1;
	    for (unsigned idx = 0
		       ; (rc != 0) && (idx < vpip_module_path_cnt)
		       ;  idx += 1) {
		  export_flag = false;
		  sprintf(buf, "%s%c%s.vpi", vpip_module_path[idx], sep, name);
		  rc = stat(buf,&sb);

		  if (rc != 0) {
			export_flag = true;
			sprintf(buf, "%s%c%s.vpl",
				vpip_module_path[idx], sep, name);
			rc = stat(buf,&sb);
		  }
	    }

	    if (rc != 0) {
		  fprintf(stderr, "%s: Unable to find a "
			  "`%s.vpi' module on the search path.\n",
			  name, name);
		  return;
	    }

      }

      vpi_mode_flag = VPI_MODE_REGISTER;
      for ( unsigned tmp = 0 ;  vlog_startup_routines[tmp] ;  ++tmp )
         vlog_startup_routines[tmp]();
      vpi_mode_flag = VPI_MODE_NONE;
}
