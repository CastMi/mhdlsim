#ifndef ICARUSSIMULATOR_H
#define ICARUSSIMULATOR_H

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

#include "vvp_config.h"
#include "simulator.h"
#include "observer.h"
#include <fstream>
#include <type_traits>
#include <vector>
#include <limits>

class event_s;

class IcarusSimulator : public virtual Simulator, public virtual Observer {
   static_assert( std::numeric_limits<sim_time_t>::is_integer, "sim_time_t is not an integer type" );
   static_assert( !std::numeric_limits<sim_time_t>::is_signed, "sim_time_t is signed" );
   static_assert( sizeof(sim_time_t) >= sizeof(vvp_time64_t), "sim_time_t to vvp_time64_t conversion assumption is not satisfied" );
   public:
      IcarusSimulator();
      virtual ~IcarusSimulator();

      virtual int initialize();

      virtual void notify( const Net* );

      virtual SimResult* step_event();

      virtual bool other_event();

      virtual sim_time_t next_event() const;

      virtual sim_time_t current_time() const;

      virtual void end_simulation();

      virtual SimResult* advance_time( const sim_time_t );

      // Icarus specific function
      virtual void update( vvp_net_t* );

   private:
      // Methods
      std::vector<Net*>* get_changed();
      void print_value( vvp_net_t* );
      void register_observers();
      int read_mixed();
      // Members
      vvp_time64_t schedule_time_;
      std::vector<vvp_net_t*> changed_;
      bool is_finished_;
      bool run_finals_;
      bool is_ignore_notify_;
      std::fstream mix;
};

#endif /* ICARUSSIMULATOR_H */
