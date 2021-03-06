/*
 * Copyright (c) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef MANAGER_H
#define MANAGER_H

#include "compiler_interface.h"
#include "common_data_types.hpp"
#include <string>
#include <vector>

class Simulator;
class Net;
class ModuleInstance;

/**
 * @brief Class responsible for mixed-language elaboration and simulation.
 * Creates simulator instances as needed and coordinates them.
 */
class Manager {
public:
    Manager();
    virtual ~Manager();

    /**
     * @brief Adds a new simulator instance to be managed.
     * @param comp is the instance to be added.
     */
    void add_instance( Compiler* comp );

    /**
     * @brief Starts the simulation.
     * @param step is when we have to stop.
     * @return 0 if success. Non zero value in case of failure.
     */
    int run( CompilerStep );

    // TODO interface to inspect signals, variables, control the sim flow?

private:
    // Helper functions
    void error_message( const std::string& errormsg = "" ) const;
    int do_elaboration();
    int do_analysis();
    int do_simulation();
    void end_simulation();
    int elaborate( ModuleInstance* mod_inst = nullptr );
    void add_not_found( const std::string& );
    bool set_variable( sim_time_t& );

    ///> Instances of simulators to manage.
    std::vector<Compiler*> instances_;

    ///> Currently active Compiler (i.e. executing code)
    Compiler* current_comp_;

    CompilerStep current_step_;

    std::string not_found_;
};

#endif /* MANAGER_H */
