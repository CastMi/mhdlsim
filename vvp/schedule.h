#ifndef IVL_schedule_H
#define IVL_schedule_H
/*
 * Copyright (c) 2001-2014 Stephen Williams (steve@icarus.com)
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

# include  "vthread.h"
# include  "vvp_net.h"
# include  "array.h"

/*
 * The event_s and event_time_s structures implement the Verilog
 * stratified event queue.
 *
 * The event_time_s objects are one per time step. Each time step in
 * turn contains a list of event_s objects that are the actual events.
 *
 * The event_s objects are base classes for the more specific sort of
 * event.
 */
struct event_s {
      struct event_s*next;
      virtual ~event_s() { }
      virtual void run_run(void) =0;
      virtual const char* name(void) const = 0;

	// Write something about the event to stderr
      virtual void single_step_display(void);

	// Fallback new/delete
      static void* operator new (size_t size) { return ::new char[size]; }
      static void  operator delete(void*ptr)  { ::delete[]( (char*)ptr ); }
};

struct assign_vector4_event_s  : public event_s {
	/* The default constructor. */
      explicit assign_vector4_event_s(const vvp_vector4_t&that) : val(that) {
	    base = 0;
	    vwid = 0;
      }
      const char* name(void) const { return "assign_vector4_event_s"; }

	/* Where to do the assign. */
      vvp_net_ptr_t ptr;
	/* Value to assign. */
      vvp_vector4_t val;
	/* Offset of the part into the destination. */
      unsigned base;
	/* Width of the destination vector. */
      unsigned vwid;
      void run_run(void);
      void single_step_display(void);

      static void* operator new(size_t);
      static void operator delete(void*);
};

extern unsigned long count_time_events;
struct event_time_s {
      event_time_s() {
	    count_time_events += 1;
	    start = 0;
	    active = 0;
	    nbassign = 0;
	    rwsync = 0;
	    rosync = 0;
	    del_thr = 0;
	    next = NULL;
      }
      vvp_time64_t delay;

      struct event_s*start;
      struct event_s*active;
      struct event_s*nbassign;
      struct event_s*rwsync;
      struct event_s*rosync;
      struct event_s*del_thr;

      struct event_time_s*next;

      static void* operator new (size_t);
      static void operator delete(void*obj, size_t s);
};

/*
 * This class supports the propagation of vec4 outputs from a
 * vvp_net_t object.
 */
struct propagate_vector4_event_s : public event_s {
	/* The default constructor. */
      explicit propagate_vector4_event_s(const vvp_vector4_t&that) : val(that) {
	    net = NULL;
      }
	/* A constructor that makes the val directly. */
      propagate_vector4_event_s(const vvp_vector4_t&that, unsigned adr, unsigned wid)
      : val(that,adr,wid) {
	    net = NULL;
      }
      const char* name(void) const { return "propagate_vector4_event_s"; }

	/* Propagate the output of this net. */
      vvp_net_t*net;
	/* value to propagate */
      vvp_vector4_t val;
	/* Action */
      void run_run(void);
      void single_step_display(void);
};

/*
 * This class supports the propagation of real outputs from a
 * vvp_net_t object.
 */
struct propagate_real_event_s : public event_s {
      const char* name(void) const { return "propagate_real_event_s"; }
	/* Propagate the output of this net. */
      vvp_net_t*net;
	/* value to propagate */
      double val;
	/* Action */
      void run_run(void);
      void single_step_display(void);
};

/*
 * This causes a thread to be scheduled for execution. The schedule
 * puts the event into the event queue after any existing events for a
 * given time step. The delay is a relative time.
 *
 * If the delay is zero, the push_flag can be used to force the event
 * to the front of the queue. %fork uses this to get the thread
 * execution ahead of non-blocking assignments.
 */
extern void schedule_vthread(vthread_t thr, vvp_time64_t delay,
			     bool push_flag =false);

extern void schedule_final_vthread(vthread_t thr);

void schedule_event_push_(struct event_s*);

/*
 * Create an assignment event. The val passed here will be assigned to
 * the specified input when the delay times out. This is scheduled
 * like a non-blocking assignment. This is in fact mostly used to
 * implement the non-blocking assignment.
 */
extern void schedule_assign_vector(vvp_net_ptr_t ptr,
				   unsigned base, unsigned vwid,
				   const vvp_vector4_t&val,
				   vvp_time64_t  delay);

extern void schedule_assign_array_word(vvp_array_t mem,
				       unsigned word_address,
				       unsigned off,
				       vvp_vector4_t val,
				       vvp_time64_t delay);

extern void schedule_assign_array_word(vvp_array_t mem,
				       unsigned word_address,
				       double val,
				       vvp_time64_t delay);
/*
 * Create an event to propagate the output of a net.
 */
extern void schedule_propagate_vector(vvp_net_t*ptr,
				      vvp_time64_t  delay,
				      const vvp_vector4_t&val);

/*
 * This is very similar to schedule_assign_vector, but generates an
 * event in the active queue. It is used at link time to assign a
 * constant value (i.e. C4<...>) to the input of a functor. This
 * creates an event in the active queue.
 */
extern void schedule_set_vector(vvp_net_ptr_t ptr, const vvp_vector4_t&val);
extern void schedule_set_vector(vvp_net_ptr_t ptr, vvp_vector8_t val);
extern void schedule_set_vector(vvp_net_ptr_t ptr, double val);

/*
 * The schedule_init_vector function assigns an initial value to a
 * functor. The assignment is put into a pre-simulation queue that is
 * run before the rest of the simulation runs. This is used to assign
 * initial values to variables and have them propagate through the
 * net. Doing the init before the rest of the scheduler starts
 * prevents threads being triggered by the initialization of the
 * variable, but still allows the initial value to be driven
 * (propagated as events) through the rest of the net.
 */
extern void schedule_init_vector(vvp_net_ptr_t ptr, vvp_vector4_t val);
extern void schedule_init_vector(vvp_net_ptr_t ptr, vvp_vector8_t val);
extern void schedule_init_vector(vvp_net_ptr_t ptr, double val);
/*
 * The schedule_init_propagate function is similar to the above but
 * propagates an initial value from a net output (i.e. without passing
 * through the net functor).
 */
extern void schedule_init_propagate(vvp_net_t*net, vvp_vector4_t val);
extern void schedule_init_propagate(vvp_net_t*net, double val);

/*
 * Create a generic event. This supports scheduled events that are not
 * any of the specific types above. The vvp_get_event_t points to a
 * function to be executed when the scheduler gets to the event. It is
 * up to the user to allocate/free the vvp_get_event_s object. The
 * object is never referenced by the scheduler after the run method is
 * called.
 *
 * The sync_flag is true if this is intended to be a sync event. These
 * are placed in the stratified event queue after nb assigns. If the
 * ro_flag is true as well, then it is a Read-only sync event, with
 * all that means.
 *
 * If the sync_flag is false, then the event is ACTIVE, and the
 * ro_flag is ignored.
 */

typedef struct vvp_gen_event_s *vvp_gen_event_t;

extern void schedule_generic(vvp_gen_event_t obj, vvp_time64_t delay,
			     bool sync_flag, bool ro_flag =true,
			     bool delete_obj_when_done =false);

/* Create a functor output event. This is placed in the pre-simulation
 * event queue if the scheduler is still processing pre-simulation
 * events, otherwise it is placed in the stratified event queue as an
 * ACTIVE event with a delay of 0. It is up to the user to allocate/free
 * the vvp_get_event_s object. The object is never referenced by the
 * scheduler after the run method is called.
*/
extern void schedule_functor(vvp_gen_event_t obj);

extern void schedule_at_start_of_simtime(vvp_gen_event_t obj, vvp_time64_t delay);

/* Use this is schedule thread deletion (after rosync). */
extern void schedule_del_thr(vthread_t thr);

extern bool schedule_single_step_flag;
extern void signals_revert(void);

struct vvp_gen_event_s
{
      virtual ~vvp_gen_event_s() =0;
      virtual void run_run() =0;
      virtual void single_step_display(void);
};

/*
 * Get the current absolute simulation time. This is not used
 * internally by the scheduler (which uses time differences instead)
 * but is used for printouts and stuff.
 */
extern vvp_time64_t schedule_simtime(void);

extern void signals_capture(void);
extern void run_rosync(struct event_time_s*ctim);

/*
 * This function is the equivalent of the $finish system task. It
 * tells the simulator that simulation is done, the current thread
 * should be stopped, all remaining events abandoned and the
 * schedule_simulate() function will return.
 *
 * The schedule_finished() function will return true if the
 * schedule_finish() function has been called.
 */
extern void schedule_finish(int rc);
extern void schedule_stop(int rc);
extern void schedule_single_step(int flag);
extern bool schedule_finished(void);
extern bool schedule_stopped(void);

/*
 * The scheduler calls this function to process stop events. When this
 * function returns, the simulation resumes.
 */
extern void stop_handler(int rc);

/*
 * These are event counters for the sake of performance measurements.
 */
extern unsigned long count_assign_events;
extern unsigned long count_gen_events;
extern unsigned long count_prop_events;
extern unsigned long count_thread_events;
extern unsigned long count_event_pool;

#endif /* IVL_schedule_H */
