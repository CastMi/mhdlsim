#ifndef OBSERVABLE_H
#define OBSERVABLE_H

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

#include "vpi_user.h"
#include "observer.h"
#include "vvp_net.h"
#include <vector>
#include <algorithm>

class Observable {
   public:
      void attach( Observer *obs ) {
         if( std::find(views_.begin(), views_.end(), obs) == views_.end() )
            views_.push_back(obs);
      }
      void notify( vvp_net_t* tmp ) {
         for( auto& elem : views_ )
            elem->update( tmp );
      }
   protected:
      Observable() : views_() {};
   private:
      std::vector<Observer*> views_;
};

#endif /* OBSERVABLE_H */
