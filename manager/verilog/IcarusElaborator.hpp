#ifndef ICARUSELABORATOR_H
#define ICARUSELABORATOR_H

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

#include "module.h"
#include "IcarusHandler.hpp"
#include "elaborator.h"
#include "netlist.h"

class PGModule;
class Module;

class IcarusElaborator : public virtual Elaborator, public virtual IcarusHandler {
   public:
      enum verilog_logic { VERILOGX, VERILOG0, VERILOG1, VERILOGZ };
      enum vhdl_logic { VHDLU, VHDLX, VHDL0, VHDL1, VHDLZ, VHDLW, VHDLL, VHDLH, VHDLDASH };
      enum vhdl_strenght { WEAK, STRONG };

      IcarusElaborator();
      virtual ~IcarusElaborator();

      virtual ModuleSpec* elaborate( ModuleInstance* = nullptr );

      bool can_continue() const;

      int emit_code();

      ElabResult* instantiate( ModuleSpec& );

   protected:
      ///> Modules provided by this simulator instance. They can be instantiated
      ///> as required. The string key is the name of the module, as defined
      ///> in its interface.
      std::map<const std::string, ModuleInterface> modules_;

      ModuleSpec* create_spec( const PGModule*, NetScope* );
      ModuleSpec* continue_elaboration( ModuleInstance* );
      ModuleSpec* start_elaboration();
      Module* mod_from_spec( ModuleSpec* );
      void create_and_substitute_pgmodule( ModuleInstance*, NetScope* );

      vhdl_strenght transform( ivl_drive_t );
      void transform( ivl_variable_type_t );
      verilog_logic transform( vhdl_logic );
      ModuleInstance* create_instance( ModuleSpec& );
      void add_vpi_module( const char* );

      Design* des_;

      ///> Instances of modules handled by the simulator instance. The string key
      ///> is the name of an instance, not the name of the module.
      std::map<const std::string, std::vector<std::string>> instances_;
   private:
      vector<struct root_elem> root_elems_;
      vector<struct pack_elem> pack_elems_;
      std::map<std::string, std::vector<std::string> > mixed_borders_;
      map<string, const char*> f_;
      char *vpi_module_list_;
};

#endif /* ICARUSELABORATOR_H */
