/*
 * Copyright (c) 2016 CERN
 * @author Michele Castellana <michele.castellana@cern.ch>
 *
 * This source code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IcarusHandler.hpp"
#include "compiler.h"
#include <cassert>

bool debug_elaborate = true;
bool warn_ob_select = false;
unsigned integer_width = 32;
unsigned width_cap = 65536;
char depfile_mode = 'a';
bool warn_implicit  = false;
bool warn_implicit_dimensions = false;
bool warn_timescale = false;
bool warn_portbinding = false;
bool warn_inf_loop = false;
bool warn_sens_entire_vec = false;
bool warn_sens_entire_arr = false;
bool warn_anachronisms = false;
char*ivlpp_string = 0;
list<perm_string> roots;
list<const char*> library_suff;
int def_ts_prec = 0;
int def_ts_units = 0;
/* Dependency file output mode */
char dep_mode = 'a';
/* Path to vhdlpp */
char *vhdlpp_path = 0;
/* vhdlpp work directory */
char *vhdlpp_work = 0;

char**vhdlpp_libdir = 0;
unsigned vhdlpp_libdir_cnt = 0;

char**include_dir = 0;
unsigned include_cnt = 0;

int relative_include = 0;

int line_direct_flag = 0;

FILE *depend_file = NULL;
bool verbose_flag = false;

/*
 * For some generations we allow a system function to be called
 * as a task and only print a warning message. The default for
 * this is that it is a run time error.
 */
ivl_sfunc_as_task_t def_sfunc_as_task = IVL_SFUNC_AS_TASK_ERROR;

/*
 * Optimization control flags.
 */
unsigned opt_const_func = 0;

/*
 * Are we doing synthesis?
 */
bool synthesis = false;

/*
 * Miscellaneous flags.
 */
bool disable_virtual_pins = false;
unsigned long array_size_limit = 16777216;  // Minimum required by IEEE-1364?
unsigned recursive_mod_limit = 10;
bool disable_concatz_generation = false;

map<perm_string,unsigned> missing_modules;
map<perm_string,bool> library_file_map;

/*
 * Debug message class flags.
 */
bool debug_scopes = true;
bool debug_eval_tree = true;
bool debug_emit = true;
bool debug_synth2 = true;
bool debug_optimizer = true;

/*
 * Keep a heap of identifier strings that I encounter. This is a more
 * efficient way to allocate those strings.
 */
StringHeapLex lex_strings;
StringHeapLex filename_strings;
StringHeapLex bits_strings;

/*
 * These are the language support control flags. These support which
 * language features (the generation) to support. The generation_flag
 * is a major mode, and the gn_* flags control specific sub-features.
 */
generation_t generation_flag = GN_VER2009;
bool gn_icarus_misc_flag = true;
bool gn_cadence_types_flag = true;
bool gn_specify_blocks_flag = true;
bool gn_assertions_flag = true;
bool gn_io_range_error_flag = true;
bool gn_strict_ca_eval_flag = false;
bool gn_strict_expr_width_flag = false;
bool gn_verilog_ams_flag = false;

const char* targ = nullptr;

int e_flag = 0;

IcarusHandler::IcarusHandler() {}

IcarusHandler::~IcarusHandler() {}

void
IcarusHandler::add_files( std::vector<std::string>& in_files ) {
   assert( in_files.size() );
   files_.insert(in_files.begin(), in_files.end());
};

void
IcarusHandler::add_file( const std::string& newfile ) {
   assert( !newfile.empty() );
   auto result = files_.insert(newfile);
   assert( result.second );
}

void
IcarusHandler::processParams( std::vector<std::string>& params_ ) {
   StringHeapLex lex_strings;
   for(auto it = params_.begin(); it != params_.end(); it++) {
      if( !(*it).compare("-V") ) { 
         // TODO: print  the correct version
         std::cout << "Here you should see the version informations" << std::endl;
         return;
      }
      if( !(*it).compare("-E") ) {
         e_flag = 1;
         continue;
      }
      if( !(*it).compare("-v") ) {
         verbose_flag = true;
         continue;
      }
      if( !(*it).substr(0, 2).compare("-s") ) {
         std::string tmp((*it).substr(2));
         tmp.erase(std::remove( tmp.begin(), tmp.end(), ' ' ), tmp.end());
         roots.push_back( lex_strings.make( tmp.c_str() ) );
         continue;
      }
      if( !(*it).substr(0, 2).compare("-t") ) {
         std::string tmp = (*it).substr(2).c_str();
         tmp.erase(std::remove( tmp.begin(), tmp.end(), ' ' ), tmp.end());
         targ = tmp.c_str();
         continue;
      }
      // This should never happen. The only 2 possible reasons are:
      // 1 - The ArgumentParser did a wrong translation of parameters.
      // 2 - There is an unhandled parameter.
      assert(false);
      std::cerr << "ERROR: paramenter not recognized" << std::endl;
      abort();
   }
   // Set the default standard
   lexor_keyword_mask = GN_KEYWORDS_1364_1995
                       |GN_KEYWORDS_1364_2001
                       |GN_KEYWORDS_1364_2001_CONFIG
                       |GN_KEYWORDS_1364_2005
                       |GN_KEYWORDS_1800_2005
                       |GN_KEYWORDS_1800_2009
                       |GN_KEYWORDS_1800_2012;
}
