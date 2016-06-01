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

#include <iostream>
#include <string>
#include "IcarusSimulator.hpp"
#include "sim_result.hpp"
#include "net.h"
#include "schedule.h"
#include "compile.h"
#include "vvp_object.h"
#include "vpi_priv.h"
#include "vpi_mcd.h"
#include "vvp_vpi.h"
#include "parse_misc.h"
#include "vpi_callback.h"
#include "vvp_cleanup.h"
#include "vvp_net_sig.h"

#define VVP_INPUT "myoutput"
#define MIXED_FILENAME "mixed_file"

bool have_ivl_version;
// TODO: to assign
int vpip_delay_selection;
int vvp_return_value;
extern bool sim_started;
extern struct event_time_s* sched_list;
extern struct event_s* schedule_init_list;
extern struct event_s* schedule_final_list;
extern bool schedule_runnable;
extern bool schedule_stopped_flag;
// map<vv_net_t, signal>
extern std::map< vvp_net_t*, std::string > handles;
// map<scope_name, signal_names>
extern std::map<const std::string, std::vector<std::string>> instances;

IcarusSimulator::IcarusSimulator()
   : schedule_time_(),
   changed_(),
   is_finished_(false),
   run_finals_(false),
   is_ignore_notify_(false) {
      sim_started = false;
};
IcarusSimulator::~IcarusSimulator() {
   if( mix.is_open() )
      mix.close();
};

int
IcarusSimulator::initialize() {
   read_mixed();
   vpip_mcd_init( NULL );
   vpi_mcd_printf( 1, "Compiling VVP ...\n" );
   vvp_vpi_init();
   //vpi_set_vlog_info( 0, 0 );
   compile_init();
   int ret = compile_design( VVP_INPUT );
   for( const auto& elem : instances ) {
      std::cerr << "There are mixed signals that are not traced in the module "
         << elem.first
         << ". Unless you really know what you are doing, \
this scenario should never happen. The signals are:\n";
      for( const auto& signal : elem.second ) {
         std::cerr << signal << std::endl;
      }
   }
   vvp_destroy_lexor();
   print_vpi_call_errors();
   if( ret )
      return ret;
   compile_cleanup();
   if( compile_errors ) {
      vpi_mcd_printf(1, "%s: Program not runnable, %u errors.\n",
            VVP_INPUT, compile_errors);
      return compile_errors;
   }

   schedule_time_ = 0;
   if( verbose_flag ) {
      vpi_mcd_printf(1, " ...execute EndOfCompile callbacks\n");
   }
   vpiEndOfCompile();

   if( verbose_flag ) {
      vpi_mcd_printf(1, " ...propagate initialization events\n");
   }
   // Execute initialization events.
   while (schedule_init_list) {
      struct event_s*cur = schedule_init_list->next;
      if (cur->next == cur) {
         schedule_init_list = 0;
      } else {
         schedule_init_list->next = cur->next;
      }
      cur->run_run();
      delete cur;
   }
   if( verbose_flag ) {
      vpi_mcd_printf(1, " ...execute StartOfSim callbacks\n");
   }
   // Execute start of simulation callbacks
   vpiStartOfSim();
   sim_started = true;
   signals_capture();
   run_finals_ = schedule_runnable;
   register_observers();
   return 0;
}

void
IcarusSimulator::register_observers() {
   for( const auto& elem : handles ) {
      elem.first->attach( this );
      if( verbose_flag )
         std::cerr << elem.second << " attached" << std::endl;
   }
}

int
IcarusSimulator::read_mixed() {
   mix.open(MIXED_FILENAME, std::fstream::in);
   std::string tmp;
   std::string scope_name;
   for( std::getline( mix, tmp );
         !mix.eof();
         std::getline( mix, tmp ) ) {
      tmp.erase(0, tmp.find_first_not_of(' '));
      tmp.erase(tmp.find_last_of(';'));
      std::size_t divider = tmp.find_first_of(' ');
      if( divider != std::string::npos ) {
         assert(tmp.find("scope") != std::string::npos);
         scope_name = tmp.substr( divider + 1 );
         tmp.clear();
      } else {
         assert(!scope_name.empty()); 
         instances[ scope_name ].push_back( tmp );
         debug_output( std::string("We are now tracing ")
               + scope_name
               + "."
               + tmp );
      }
   }
   mix.close();
   return 0;
}

bool
IcarusSimulator::other_event() {
   if( !sim_started )
      return schedule_init_list == nullptr;
   return !schedule_finished() && !is_finished_;
}

void
IcarusSimulator::print_value( vvp_net_t* node ) {
   struct t_vpi_value tmp;
   tmp.format = vpiBinStrVal;
   dynamic_cast<vvp_signal_value*>(node->fil)->get_signal_value( &tmp );
   switch( tmp.format ) {
      case vpiIntVal:
         if( verbose_flag ) {
            std::cerr << handles[node]
               << " has now value "
               << tmp.value.integer << std::endl;
         }
         break;
      case vpiBinStrVal:
      case vpiOctStrVal:
      case vpiHexStrVal:
      case vpiDecStrVal:
      case vpiStringVal:
         if( verbose_flag ) {
            std::cerr << handles[node]
               << " has now value "
               << tmp.value.str << std::endl;
         }
         break;
      case vpiScalarVal:
         if( verbose_flag ) {
            std::cerr << handles[node]
               << " has now value "
               << std::to_string(tmp.value.scalar) << std::endl;
         }
         break;
      case vpiVectorVal:
         if( verbose_flag ) {
            std::cerr << handles[node]
               << " has now value "
               << std::to_string(tmp.value.vector->aval)
               << std::to_string(tmp.value.vector->bval) << std::endl;
         }
         break;
      case vpiRealVal:
         if( verbose_flag ) {
            std::cerr << handles[node]
               << " has now value "
               << std::to_string(tmp.value.real) << std::endl;
         }
         break;
      case vpiObjTypeVal:
      case vpiSuppressVal:
      case vpiStrengthVal:
      default:
         std::cerr << "received a print_value(vvp_net_t*) with an unsupported format"
            << std::endl;
         assert( false );
         break;
   }
}

void
IcarusSimulator::update( vvp_net_t* updated ) {
   if( is_ignore_notify_ )
      return;
   assert( handles.find(updated) != handles.end() );
   if( std::find( changed_.begin(), changed_.end(), updated ) == changed_.end() )
      changed_.push_back( updated );
   print_value( updated );
}

std::vector<Net*>*
IcarusSimulator::get_changed() {
   if( changed_.empty() )
      return nullptr;
   std::vector<Net*>* ret_val = new std::vector<Net*>;
   for( const auto& elem : changed_ ) {
      const auto string_to_manipulate = handles[elem];
      size_t divider = string_to_manipulate.find_last_of( '.' );
      assert( divider != std::string::npos );
      std::string basename = string_to_manipulate.substr( 0, divider );
      std::string sig_name = string_to_manipulate.substr( divider + 1 );
      ret_val->push_back( new Net( sig_name, basename ) );
   }
   return ret_val;
}

void
IcarusSimulator::notify( const Net* net ) {
   assert( sim_started );
   assert( net );
   bool is_found = false;
   is_ignore_notify_ = true;
   struct __vpiSignal* sig = nullptr;
   __vpiHandle**table;
   unsigned ntable;
   vpip_make_root_iterator(table, ntable);
   for( unsigned tmp = 0; !is_found && tmp < ntable; ++tmp ) {
      __vpiScope* root = dynamic_cast<__vpiScope*>(table[tmp]);
      if( !root )
         continue;
      __vpiScope* instance = root->find_recursively(net->instance_name());
      if( !instance )
         continue;
      for( const auto& elem : instance->intern ) {
         switch( elem->get_type_code()) {
            case vpiReg:
            case vpiIntegerVar:
            case vpiNet:
            case vpiByteVar:
            case vpiBitVar:
            case vpiShortIntVar:
            case vpiIntVar:
            case vpiLongIntVar:
               {
                  sig = dynamic_cast<__vpiSignal*>( elem );
                  assert(sig);
                  if( !net->name().compare( sig->id.name ) ) {
                     std::cerr << std::string("sig name ")
                           << sig->id.name
                           << " net name " << net->name() << std::endl;
                     struct propagate_vector4_event_s* tmp = new propagate_vector4_event_s( vvp_vector4_t(net->width(), 1));
                     tmp->net = sig->node;
                     schedule_event_push_( tmp );
                     is_found = true;
                     //sig->node->send_vec4(vvp_vector4_t(1, 0), 0);
                  }
               }
               break;
            default:
               break;
         }
      }
   }
   if( verbose_flag && is_found ) {
      std::cerr << std::string(sig->id.name)
         << " not found" std::endl;
   }
   is_ignore_notify_ = false;
}

SimResult*
IcarusSimulator::step_event() {
   if( verbose_flag )
      std::cerr << "This is the start of a new step"
         << std::endl;
   assert( sim_started );
   changed_.clear();
   if( schedule_finished() ) {
      // If the manager calls this function when the simulator does not
      // have events to execute it means that something went terribly wrong
      // and therefore is better to return an error
      return new SimResult(SimResult::ERROR);
   }

   if ( schedule_stopped_flag ) {
      schedule_stopped_flag = false;
      stop_handler(0);
      // You can finish from the debugger without a time change.
      if ( !schedule_runnable ) {
         is_finished_ = true;
         return new SimResult( SimResult::OK);
      }
      return new SimResult(SimResult::OK);
   }
   struct event_time_s* ctim = sched_list;
   if (ctim->delay > 0) {

      if ( !schedule_runnable ) {
         is_finished_ = true;
         if( changed_.empty() )
            return new SimResult(SimResult::OK);
         else
            new SimResult(SimResult::CHANGED, get_changed());
      }
      schedule_time_ += ctim->delay;
      /* When the design is being traced (we are emitting
       * file/line information) also print any time changes. */
      if (vvp_show_file_line) {
         std::cerr << "Advancing to simulation time: "
            << schedule_time_ << std::endl;
      }
      ctim->delay = 0;

      vpiNextSimTime();
      // Process the cbAtStartOfSimTime callbacks.
      while (ctim->start) {
         struct event_s*cur = ctim->start->next;
         if (cur->next == cur) {
            ctim->start = 0;
         } else {
            ctim->start->next = cur->next;
         }
         if( verbose_flag ) {
            std::cerr << std::string("Executing ")
               << cur->name() << std::cerr;
         }
         cur->run_run();
         delete (cur);
      }
   }
   /* If there are no more active events, advance the event
      queues. If there are not events at all, then release
      the event_time object. */
   if (ctim->active == 0) {
      ctim->active = ctim->nbassign;
      ctim->nbassign = 0;

      if (ctim->active == 0) {
         ctim->active = ctim->rwsync;
         ctim->rwsync = 0;

         /* If out of rw events, then run the rosync
            events and delete this time step. This also
            deletes threads as needed. */
         if (ctim->active == 0) {
            run_rosync(ctim);
            sched_list = ctim->next;
            delete ctim;
            if( changed_.empty() )
               return new SimResult(SimResult::OK);
            else
               new SimResult(SimResult::CHANGED, get_changed());
         }
      }
   }
   /* Pull the first item off the list. If this is the last
      cell in the list, then clear the list. Execute that
      event type, and delete it. */
   struct event_s*cur = ctim->active->next;
   if (cur->next == cur) {
      ctim->active = 0;
   } else {
      ctim->active->next = cur->next;
   }

   if ( schedule_single_step_flag ) {
      cur->single_step_display();
      schedule_stopped_flag = true;
      schedule_single_step_flag = false;
   }

   if( verbose_flag ) {
      std::cerr << std::string("Executing ")
         << cur->name() << std::endl;
   }
   cur->run_run();

   delete (cur);
   if( changed_.empty() )
      return new SimResult(SimResult::OK);
   else
      return new SimResult(SimResult::CHANGED, get_changed());
}

void
IcarusSimulator::end_simulation() {
   assert( sim_started );
   changed_.clear();
   // Execute final events.
   schedule_runnable = run_finals_;
   while (schedule_runnable && schedule_final_list) {
      struct event_s*cur = schedule_final_list->next;
      if (cur->next == cur) {
         schedule_final_list = 0;
      } else {
         schedule_final_list->next = cur->next;
      }
      cur->run_run();
      delete cur;
   }
   signals_revert();
   vpiPostsim();
   vvp_object::cleanup();
   load_module_delete();
}

sim_time_t
IcarusSimulator::next_event() const {
   if( !sim_started )
      return IcarusSimulator::maxSimValue();
   return schedule_simtime() + sched_list->delay;
}

sim_time_t
IcarusSimulator::current_time() const {
   if( !sim_started )
      return minSimValue();
   return static_cast<sim_time_t>(schedule_simtime());
}

SimResult*
IcarusSimulator::advance_time( sim_time_t time_new ) {
   assert( sim_started );
   assert( time_new < Simulator::maxSimValue() );
   while( other_event() && time_new > next_event() ) {
      SimResult* tmp = step_event();
      switch( tmp->result() ){
         case SimResult::OK:
            break;
         case SimResult::CHANGED:
         case SimResult::ERROR:
            return tmp;
         default:
            assert(false);
      }
   }
   assert( next_event() >= time_new );
   return new SimResult();
}
