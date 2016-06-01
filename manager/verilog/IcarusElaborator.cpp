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

#include "IcarusElaborator.hpp"
#include "elab_result.hpp"
#include <vector>
#include <list>
#include <cstring>
#include <string>
#include "net.h"
#include "StringHeap.h"
#include "PPackage.h"
#include "elaborate.hh"
#include "PExpr.h"
#include "PWire.h"
#include "netvector.h"
#include "pform.h"
#include "HName.h"
#include "parse_api.h"
#include "Module.h"
#include "PGate.h"
#include "t-dll.h"
#include "vpi_priv.h"

#define VVP_INPUT "myoutput"
#define MIXED_FILENAME "mixed_file"

extern std::map<perm_string, PPackage*> pform_packages;
extern std::map<perm_string, unsigned> missing_modules;
extern std::list<perm_string> roots;
extern bool debug_elaborate;
extern dll_target dll_target_obj;
const char* basedir = strdup(".");
// map<name_of_the_module, pair< module_instance, father_scope >>
std::map<perm_string, std::pair<const PGModule*,NetScope*>> missing_specs;
std::map<perm_string, Module*> fake_modules;

IcarusElaborator::IcarusElaborator()
   : root_elems_(),
   pack_elems_(),
   mixed_borders_(),
   f_(),
   vpi_module_list_(nullptr) {
      // FIXME: temporary hard-coded paths
      f_["DLL"] = "/usr/local/lib/ivl/vvp.tgt";
      f_["VVP_EXECUTABLE"] = "/usr/local/bin/vvp";
      f_["-o"] = VVP_INPUT ;
      vpip_module_path[0] = "/usr/local/lib/ivl";
      ++vpip_module_path_cnt;
};
IcarusElaborator::~IcarusElaborator() {
   if(des_) {
      delete des_;
      des_ = nullptr;
   }
   if( mix.is_open() )
      mix.close();
};

int
IcarusElaborator::emit_mixed_file() {
   mix.open(MIXED_FILENAME, std::fstream::out | std::fstream::trunc);
   for( const auto& it_ext : mixed_borders_ ) {
      mix << "scope " << it_ext.first << ";" << std::endl;
      for( const auto& it_int : it_ext.second ) {
         mix << it_int << ";" << std::endl;
      }
   }
   mix.close();
   return 0;
}

int
IcarusElaborator::emit_code() {
   if(can_continue()) {
      // this is probably not the best place ever to add
      // the set_flag but at least I am sure it will be executed
      // at the end of the elaboration
      assert(des_);
      des_->set_flags(f_);
      if (debug_elaborate) {
         debug_output( "Start code generation" );
      }
      return des_->emit(&dll_target_obj) || emit_mixed_file();
   }
   return 1;
}

inline void
IcarusElaborator::debug_output( const std::string& output ) const {
   std::cerr << "<Icarus>: elaborate: " << output << "." << std::endl;
}

bool
IcarusElaborator::can_continue() const {
   if( !missing_modules.empty() || !missing_specs.empty() ) {
      if (debug_elaborate) {
         debug_output( "I can not continue with code generation" );
      }
      return false;
   }
   return true;
}

ModuleSpec*
IcarusElaborator::create_spec( const PGModule* mod, NetScope* scope ) {
   // check the currectness of the assumptions
   assert( mod && scope );
   // instance is father of the module
   assert( pform_modules.find(scope->module_name())->second->get_gate(mod->get_name()) );

   if (debug_elaborate) {
      debug_output( std::string("Create spec for ") + mod->get_type().str() );
   }

   StringHeapLex lex_strings;
   ModuleSpec* return_val = new ModuleSpec( std::string( mod->get_type() ),
         std::string( mod->get_name() ) );
   for( unsigned i = 0; i < mod->pin_count(); ++i ) {
      PExpr* name = mod->pin(i);
      assert( name );
      assert( dynamic_cast<PEIdent*>( name ) );
      std::string string_port_name = std::string( dynamic_cast<PEIdent*>(name)->path().back().name.str() );
      assert( !string_port_name.empty() );
      // FIXME: the size, of course, is wrong
      PExpr::width_mode_t mode = PExpr::UNSIZED;
      return_val->make_port( string_port_name, name->test_width( des_, scope, mode ), Port::IN );
      //transform(name->expr_type());
   }
   assert( mod->pin_count() == return_val->size() );
   return return_val;
}

ModuleInstance*
IcarusElaborator::create_instance( ModuleSpec& spec ) {
   if (debug_elaborate) {
      debug_output( std::string("Create an instance for the spec ")
                 + spec.name() );
   }
   StringHeapLex lex_strings;
   perm_string cur_name = lex_strings.make( spec.name().c_str() );
   ModuleInstance* instance_found = nullptr;
   // FIXME: the if is here just for debugging purposes
   // It should be replace by:
   // if( pform_modules.find(cur_name) == pform_modules.end() ) {
   //   return false;
   // Module *module = pform_modules.find(cur_name)->second;
   Module *module;
   if( pform_modules.find(cur_name) == pform_modules.end() ) {
      assert( fake_modules.find(cur_name) != fake_modules.end() );
      module = fake_modules.find(cur_name)->second;
   } else {
      module = pform_modules.find(cur_name)->second;
   }
   // create a new scope to handle the instance
   //NetScope*scope = des_->make_root_scope( lex_strings.make(spec.name()), module->program_block, module->is_interface);
   instance_found = new ModuleInstance( &spec );
   // FIXME: we should do some checks here
   /*
      for( auto it = spec.ports().begin(); it != spec.ports().end(); ++it ) {
      char *abc = new char[ (*it)->name().length() + 1 ];
      strncpy( abc, (*it)->name().c_str(), (*it)->name().length() );
      PEString *port_pextring = new PEString( abc );
      tmp->push_back(port_pextring);
      NetNet* tmp = scope->find_signal( lex_strings.make( (*it)->name().c_str() ) );
      if( !tmp ) {
      if (debug_elaborate) {
      cerr << "<Icarus>: elaborate: signal " << (*it)->name().c_str() 
      << " not found" << endl;
      }
      return false;
      }
   // FIXME: this check is wrong but is here just to get the idea
   if( (*it)->width() != tmp->vector_width() ) {
   if (debug_elaborate) {
   cerr << "<Icarus>: elaborate: " << (*it)->name().c_str() 
   << " in " << cur_name.str() << " has wrong width" << endl;
   cerr << (*it)->name().c_str() << " has " << (*it)->width()
   << " whereas " << tmp->vector_width();
   }
   //return false;
   }
   }
   */
   return instance_found;
}

ElabResult*
IcarusElaborator::instantiate( ModuleSpec& spec ) {
   if (debug_elaborate) {
      debug_output( std::string("Received the spec ")
              + spec.name()
              + " for instantiation" );
   }
   StringHeapLex lex_strings;
   perm_string cur_name = lex_strings.make( spec.name().c_str() );
   if( !des_ ) {
      // Elaboration has never started
      roots.push_back( lex_strings.make( cur_name ) );
      ModuleSpec* tmp = start_elaboration();
      if( tmp )
         return new ElabResult( tmp );
   }
   // FIXME: The following if is here
   // just for testing purposes...TO REMOVE
   if( pform_modules.find(cur_name) == pform_modules.end() ) {
      Module* tmp = mod_from_spec( &spec );
      assert(tmp);
   }
   // If there was a problem in the instance creation, return a NOT_FOUND
   ModuleInstance* found = create_instance( spec );
   if ( !found )
      return new ElabResult();
   return new ElabResult(found);
}

void
IcarusElaborator::add_vpi_module( const char* name ) {
   if (vpi_module_list_ == 0) {
      vpi_module_list_ = strdup(name);
   } else {
      char*tmp = (char*)realloc(vpi_module_list_,
            strlen(vpi_module_list_)
            + strlen(name)
            + 2);
      strcat(tmp, ",");
      strcat(tmp, name);
      vpi_module_list_ = tmp;
   }
   f_["VPI_MODULE_LIST"] = vpi_module_list_;
}

ModuleSpec*
IcarusElaborator::start_elaboration() {
   assert( !roots.empty() );
   des_ = new Design;

   // Elaborate enum sets in $root scope.
   elaborate_rootscope_enumerations(des_);

   // Elaborate tasks and functions in $root scope.
   elaborate_rootscope_tasks(des_);

   // Elaborate classes in $root scope.
   elaborate_rootscope_classes(des_);

   // Elaborate the packages. Package elaboration is simpler
   // because there are fewer sub-scopes involved.
   for( const auto& pac : pform_packages ) {

      //assert(pac.second, pac.first == pac.second->pscope_name());
      NetScope*scope = des_->make_package_scope(pac.first);
      scope->set_line(pac.second);

      elaborator_work_item_t*es = new elaborate_package_t(des_, scope, pac.second);
      des_->elaboration_work_list.push_back(es);

      pack_elems_.push_back( { pac.second, scope } );
   }

   // Scan the root modules by name, and elaborate their scopes.
   for( const auto& root : roots ) {

      // Look for the root module in the list.
      map<perm_string,Module*>::const_iterator mod = pform_modules.find(root);
      if (mod == pform_modules.end()) {
         debug_output( std::string("error: Unable to find the root module \"")
                      + root.str() + "\" in the Verilog source.\n"
                      + "     : Perhaps ``-s " + root.str()
                      + "'' is incorrect?" );
         des_->errors++;
         continue;
      }

      // Get the module definition for this root instance.
      Module *rmod = (*mod).second;

      // Make the root scope. This makes a NetScope object and
      // pushes it into the list of root scopes in the Design.
      NetScope*scope = des_->make_root_scope(root, rmod->program_block,
            rmod->is_interface);

      // Collect some basic properties of this scope from the
      // Module definition.
      scope->set_line(rmod);
      scope->time_unit(rmod->time_unit);
      scope->time_precision(rmod->time_precision);
      scope->time_from_timescale(rmod->time_from_timescale);
      des_->set_precision(rmod->time_precision);


      // Save this scope, along with its definition, in the
      // "root_elems_" list for later passes.
      struct root_elem tmp = { rmod, scope };
      root_elems_.push_back( tmp );

      // Arrange for these scopes to be elaborated as root
      // scopes. Create an "elaborate_root_scope" object to
      // contain the work item, and append it to the scope
      // elaborations work list.
      elaborator_work_item_t*es = new elaborate_root_scope_t(des_, scope, rmod);
      des_->elaboration_work_list.push_back(es);
   }

   // Run the work list of scope elaborations until the list is
   // empty. This list is initially populated above where the
   // initial root scopes are primed.
   while (! des_->elaboration_work_list.empty()) {
      // Push a work item to process the defparams of any scopes
      // that are elaborated during this pass. For the first pass
      // this will be all the root scopes. For subsequent passes
      // it will be any scopes created during the previous pass
      // by a generate construct or instance array.
      des_->elaboration_work_list.push_back(new top_defparams(des_));

      // Transfer the queue to a temporary queue.
      list<elaborator_work_item_t*> cur_queue;
      while (! des_->elaboration_work_list.empty()) {
         cur_queue.push_back(des_->elaboration_work_list.front());
         des_->elaboration_work_list.pop_front();
      }

      // Run from the temporary queue. If the temporary queue
      // items create new work queue items, they will show up
      // in the elaboration_work_list and then we get to run
      // through them in the next pass.
      while (! cur_queue.empty()) {
         elaborator_work_item_t*tmp = cur_queue.front();
         cur_queue.pop_front();
         tmp->elaborate_runrun();
         delete tmp;
      }

      if (! des_->elaboration_work_list.empty()) {
         des_->elaboration_work_list.push_back(new later_defparams(des_));
      }
   }

   if (debug_elaborate) {
      debug_output( "elaboration work list done. Start processing residual defparams" );
   }

   // Look for residual defparams (that point to a non-existent
   // scope) and clean them out.
   des_->residual_defparams();

   // Errors already? Probably missing root modules. Just give up
   // now and return nothing.
   if (des_->errors > 0) {
      return nullptr;
   }

   if( !missing_specs.empty() ) {
      auto it = missing_specs.begin();
      ModuleSpec* abc = create_spec( (*it).second.first, (*it).second.second );
      assert( abc );
      return abc;
   }

   return continue_elaboration(nullptr);
}

Module*
IcarusElaborator::mod_from_spec( ModuleSpec* inst ) {
   StringHeapLex lex_strings;
   const perm_string provided = lex_strings.make( inst->name().c_str() );
   Module* ret_val = nullptr;
   if( fake_modules.find(provided) == fake_modules.end() ) {
      fake_modules[provided] = new FakeModule( nullptr, provided );
      ret_val = fake_modules[provided];
      PWire *tmp = nullptr;
      for( const auto& it : inst->ports() ) {
         // fill the port list of the module
         perm_string cur_name = lex_strings.make( it->name().c_str() );
         ret_val->ports.push_back( pform_module_port_reference(
                  lex_strings.make(it->name()),
                  "filename", 0
                  ));
         // Defined as Register otherwise during elaboration Icarus will
         // attach a fake driver to drive the Z value
         switch( it->direction() ) {
            case Port::IN:
               tmp = new PWire( cur_name, NetNet::IMPLICIT, NetNet::PINPUT, IVL_VT_NO_TYPE );
               break;
            case Port::OUT:
               tmp = new PWire( cur_name, NetNet::IMPLICIT, NetNet::POUTPUT, IVL_VT_NO_TYPE );
               break;
            case Port::INOUT:
               tmp = new PWire( cur_name, NetNet::IMPLICIT, NetNet::PINOUT, IVL_VT_NO_TYPE );
               break;
            case Port::UNKNOWN:
               tmp = new PWire( cur_name, NetNet::IMPLICIT, NetNet::NOT_A_PORT, IVL_VT_NO_TYPE );
               break;
            default:
               break;
         }
         assert(tmp);
         ret_val->wires[ cur_name ] = tmp;
         //tmp->set_wire_type(NetNet::INTEGER);
         //tmp->set_data_type();
         tmp->set_range_scalar(SR_PORT);
      }
   } else {
      ret_val = fake_modules[provided];
   }
   assert(ret_val);
   assert( inst->ports().size() == ret_val->ports.size() );
   assert( ret_val->mod_name() == provided );
   return ret_val;
}

void
IcarusElaborator::create_and_substitute_pgmodule( ModuleInstance* inst, NetScope* scope ) {
   StringHeapLex lex_strings;
   const perm_string module_name = lex_strings.make( inst->iface()->name().c_str() );
   const perm_string instance_name = lex_strings.make( inst->iface()->instance_name().c_str() );
   assert( fake_modules.find(module_name) != fake_modules.end() );
   assert( fake_modules.find(module_name)->second );
   PGModule *instance = new PGModule( fake_modules.find(module_name)->second, instance_name, true );
   assert(pform_modules.find(scope->module_name()) != pform_modules.end());
   Module *father = pform_modules[scope->module_name()];
   unsigned num = father->get_gates().size();
   static_cast<FakeModule*>(father)->remove_gate(instance_name);
   assert(father->get_gates().size() == num - 1);
   father->add_gate(instance);
   assert(father->get_gates().size() == num);
   // This will elaborate the fake module as well.
   elaborate_root_scope_t help_call(des_, scope, father);
   help_call.elaborate_runrun();
   instances_[std::string(scope->fullname().peek_name().str())].push_back(inst->iface()->instance_name());
}

ModuleSpec*
IcarusElaborator::continue_elaboration( ModuleInstance* ) {
   StringHeapLex lex_strings;
   bool rc = true;

   if (debug_elaborate) {
      debug_output( "Start calling Package elaborate_sig methods" );
   }

   // With the parameters evaluated down to constants, we have
   // what we need to elaborate signals and memories. This pass
   // creates all the NetNet and NetMemory objects for declared
   // objects.
   for( const auto& elem : pack_elems_ ) {
      PPackage*pack = elem.pack;
      NetScope*scope = elem.scope;

      if (! pack->elaborate_sig(des_, scope)) {
         if (debug_elaborate) {
            debug_output( std::string(pack->pscope_name())
                          + ": elaborate_sig failed" );
         }
         return nullptr;
      }
   }

   if (debug_elaborate) {
      debug_output( "Start calling $root elaborate_sig methods" );
   }

   des_->root_elaborate_sig();

   if (debug_elaborate) {
      debug_output( "Start calling root module elaborate_sig methods" );
   }

   for( const auto& elem : root_elems_ ) {
      Module *rmod = elem.mod;
      NetScope *scope = elem.scope;
      scope->set_num_ports( rmod->port_count() );

      if (debug_elaborate) {
         debug_output( std::string(rmod->mod_name())
                      + ": port elaboration root "
                      + std::to_string(rmod->port_count())
                      + " ports" );
      }

      if (! rmod->elaborate_sig(des_, scope)) {
         if (debug_elaborate) {
            debug_output( std::string(rmod->mod_name())
                         + ": elaborate_sig failed" );
         }
         return nullptr;
      }

      // Some of the generators need to have the ports correctly
      // defined for the root modules. This code does that.
      for (unsigned idx = 0; idx < rmod->port_count(); ++idx) {
         vector<PEIdent*> mport = rmod->get_port(idx);
         unsigned int prt_vector_width = 0;
         PortType::Enum ptype = PortType::PIMPLICIT;
         for( const auto& elem : mport ) {
            // This really does more than we need and adds extra
            // stuff to the design that should be cleaned later.
            NetNet *netnet = elem->elaborate_subport(des_, scope);
            if (netnet != 0) {
               // Elaboration may actually fail with
               // erroneous input source
               //assert(elem, netnet->pin_count()==1);
               prt_vector_width += netnet->vector_width();
               ptype = PortType::merged(netnet->port_type(), ptype);
            }
         }
         if (debug_elaborate) {
            debug_output( std::string(rmod->mod_name())
               + ": adding module port "
               + rmod->get_port_name(idx).str() );
         }
         scope->add_module_port_info(idx, rmod->get_port_name(idx), ptype, prt_vector_width );
      }
   }

   // Now that the structure and parameters are taken care of,
   // run through the pform again and generate the full netlist.

   for( const auto& elem : pack_elems_ ) {
      PPackage*pkg = elem.pack;
      NetScope*scope = elem.scope;
      rc &= pkg->elaborate(des_, scope);
   }

   des_->root_elaborate();

   for( const auto& elem : root_elems_ ) {
      Module *rmod = elem.mod;
      NetScope *scope = elem.scope;
      rc &= rmod->elaborate(des_, scope);
   }

   if (rc == false) {
      return nullptr;
   }

   // Now that everything is fully elaborated verify that we do
   // not have an always block with no delay (an infinite loop),
   // or a final block with a delay.
   if (des_->check_proc_delay() == false) {
      return nullptr;
   }

   if (debug_elaborate) {
      debug_output( std::string("Finishing with ")
         +  std::to_string(des_->find_root_scopes().size())
         + " root scopes" );
   }

   return nullptr;
}

ModuleSpec*
IcarusElaborator::elaborate( ModuleInstance* inst ) {
   if( !inst ) {
      if( !roots.empty() ) {
         if( debug_elaborate ) {
            debug_output( "Start elaboration" );
         }
         add_vpi_module( "system" );
         return start_elaboration();
      } else {
         if( debug_elaborate ) {
            debug_output( "Not a root module" );
         }
         return nullptr;
      }
   }
   if( debug_elaborate ) {
      debug_output( "Provided an instance to complete elaboration" );
   }
   StringHeapLex lex_strings;
   const perm_string provided = lex_strings.make( inst->iface()->name().c_str() );
   if( missing_specs.find( provided ) == missing_specs.end() ) {
      if( debug_elaborate ) {
         debug_output( "The instance provided was not required" );
      }
      return nullptr;
   }
   assert( fake_modules.find(provided) != fake_modules.end() );
   assert( fake_modules.find(provided)->second );
   // scope of the father
   NetScope *net = nullptr;
   for( const auto& it : missing_specs ) {
      if( it.first == provided ) {
         net = it.second.second;
         break;
      }
   }
   assert( net );
   mod_from_spec( inst->iface() );
   create_and_substitute_pgmodule( inst, net );
   assert( missing_specs.find( provided ) != missing_specs.end() );
   for( const auto& signal : inst->iface()->ports() )
      mixed_borders_[net->basename().str()].push_back(signal->name()); 
   missing_specs.erase( missing_specs.find( provided ) );
   if( !missing_specs.empty() ) {
      auto it = missing_specs.begin();
      return create_spec( (*it).second.first, (*it).second.second );
   }
   auto it = missing_specs.find( provided );
   assert( it == missing_specs.end() );
   return continue_elaboration( inst );
}

IcarusElaborator::verilog_logic
IcarusElaborator::transform(vhdl_logic type) {
   switch(type) {
      case vhdl_logic::VHDLU:
      case vhdl_logic::VHDLX:
      case vhdl_logic::VHDLDASH:
      case vhdl_logic::VHDLW:
         return verilog_logic::VERILOGX;
      case vhdl_logic::VHDL0:
         return verilog_logic::VERILOG0;
      case vhdl_logic::VHDL1:
         return verilog_logic::VERILOG1;
      case vhdl_logic::VHDLZ:
         return verilog_logic::VERILOGZ;
      case vhdl_logic::VHDLL:
         return verilog_logic::VERILOG0;
      case vhdl_logic::VHDLH:
         return verilog_logic::VERILOG1;
      default:
         // error unrecognized
         abort();
   }
}

IcarusElaborator::vhdl_strenght
IcarusElaborator::transform(ivl_drive_t type) {
   switch(type) {
      case IVL_DR_HiZ:
         // This is a special case.
         // This is a Z in std_logic
         return vhdl_strenght::STRONG;
         break;
      case IVL_DR_SUPPLY:
      case IVL_DR_PULL:
      case IVL_DR_STRONG:
         return vhdl_strenght::STRONG;
         break;
      case IVL_DR_SMALL:
      case IVL_DR_MEDIUM:
      case IVL_DR_WEAK:
      case IVL_DR_LARGE:
         return vhdl_strenght::WEAK;
         break;
      default:
         // error unrecognized
         abort();
   }
}

void
IcarusElaborator::transform(ivl_variable_type_t type) {
   switch(type) {
      case IVL_VT_REAL:
         break;
      case IVL_VT_CLASS:
      case IVL_VT_VOID:
         // error untranslatable
      default:
         // error unrecognized
         abort();
   }
}
