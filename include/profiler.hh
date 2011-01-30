/*!
 *
 * Copyright (C) 2007 Technical University of Liberec.  All rights reserved.
 *
 * Please make a following refer to Flow123d on your project site if you use the program for any purpose,
 * especially for academic research:
 * Flow123d, Research Centre: Advanced Remedial Technologies, Technical University of Liberec, Czech Republic
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 021110-1307, USA.
 *
 *
 * $Id: profiler.hh 842 2011-01-08 17:58:15Z tomas.bambuch $
 * $Revision: 842 $
 * $LastChangedBy: tomas.bambuch $
 * $LastChangedDate: 2011-01-08 18:58:15 +0100 (So, 08 led 2011) $
 *
 * @file
 * @brief ???
 *
 */


#ifndef PROFILER_H
#define	PROFILER_H

#include <map>
#include <iostream>
#include <time.h>
#include <vector>
#include <petsc.h>

#include "system.hh"

class MPI_Functions {
public:

    static int sum(int* val, MPI_Comm comm) {
        int total = 0;
        MPI_Reduce(val, &total, 1, MPI_INT, MPI_SUM, 0, comm);
        return total;
    }

    static double sum(double* val, MPI_Comm comm) {
        double total = 0;
        MPI_Reduce(val, &total, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        return total;
    }

    static int min(int* val, MPI_Comm comm) {
        int min = 0;
        MPI_Reduce(val, &min, 1, MPI_INT, MPI_MIN, 0, comm);
        return min;
    }

    static double min(double* val, MPI_Comm comm) {
        double min = 0;
        MPI_Reduce(val, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
        return min;
    }

    static int max(int* val, MPI_Comm comm) {
        int max = 0;
        MPI_Reduce(val, &max, 1, MPI_INT, MPI_MAX, 0, comm);
        return max;
    }

    static double max(double* val, MPI_Comm comm) {
        double max = 0;
        MPI_Reduce(val, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
        return max;
    }
};

/*
 * Class for profiling tree nodes.
 */
class Timer {
private:
    double start_time;
    double cumul_time;
    int count;
    int start_count;
    bool running;
    string timer_tag;
    Timer* parent_timer;
    vector<Timer*> child_timers;

    void stop(double time);

public:
    Timer(string tag, Timer* parent);
    void start(double time); // time given by Profiler
    bool end(double time);
    void forced_end(double time);
    void insert_child(Timer* child);

    /**
     * Indicates how many times the timer has been started
     */
    int call_count() const {
        return count;
    }

    /**
     * Total time measured by this timer
     */
    double cumulative_time() const {
        return cumul_time;
    }

    /**
     * Name of the timer
     */
    string tag() const {
        return timer_tag;
    }

    /**
     * Parent of the timer
     */
    Timer* parent() {
        return parent_timer;
    }
    
    vector<Timer*>* child_timers_list(){
        return &child_timers;
    }

    ~Timer();
};


/**
 *
 * @brief Main class for profiling by measuring time intervals. These time intervals
 * form a tree sturcture. The root node of the tree is automatically created and started
 * after creating the Profiler object and cannot be stopped manually.
 *
 * The class implements a singleton pattern all all the functions are accessible trough
 * Profiler::instance().
 *
 *
 */
class Profiler {
private:
    static Profiler* _instance;
    Timer *root;
    Timer *actual_node;
    clock_t start_clock;
    MPI_Comm communicator;
    int id;

    map<string, Timer*> tag_map;

    /**
     * Gets the time in milliseconds since the program was launched
     * @return time in milliseconds
     */
    double inline get_time();

    void add_timer_info(vector<vector<string>*>* timersInfo, Timer* timer);

    Profiler(MPI_Comm comm); // private constructor

    Profiler(Profiler const&); // copy constructor is private

    Profiler & operator=(Profiler const&); // assignment operator is private

    /**
     *  Pass thorugh the profiling tree (colective over processors)
     *  Print cumulative times average, balace (max/min), count (denote diferences)
     *  Destroy all structures.
     */
    ~Profiler();

public:

    /**
     * Gets the Profiler object
     */
    static Profiler* instance() {
        //singleton pattern implementation
        if (!_instance)
            _instance = new Profiler(NULL);

        return _instance;
    }

    /**
     * Destroys the Profiler object and causes that the statistics will be written to output
     */
    static void uninitialize() {
        if (_instance) {
            delete _instance;
            _instance = NULL;
        }
    }

    /**
     * Initializes the Profiler with specific MPI communicator object
     */
    static void initialize(MPI_Comm communicator) {
        if (!_instance)
            _instance = new Profiler(communicator);
    }

    /**
     * Starts a timer with specified name. If the timer is not already created, creates a new one.
     *
     * @param tag - name of the timer to start
     */
    void start(string tag);

    /**
     * Stops a timer with specified name.
     *
     * @param tag - name of the timer to stop
     */
    void end(string tag = "");

};

#define _PASTE(a,b) a ## b
#define PASTE(a,b) _PASTE(a, b)

/**
 * Starts a timer witch specified tag.
 *
 * In fact it creates an object named 'timer_' followed by the number of the line
 * where it has been used. This is done to avoid variable name conflicts when
 * using the macro more than once in one block of code.
 */
#define START_TIMER(tag) TimerFrame PASTE(timer_,__LINE__) = TimerFrame(tag)
#define END_TIMER(tag) TimerFrame::endTimer(tag)          // only if you want end on diferent place then end of function

/**
 *
 * @brief Class for automatic timer closing. This class is used by START_TIMER macro
 * and is responsible for the fact that we don't have to call END_TIMER macro to stop the timer,
 * the timer will be stopped at the end of the block in which START_TIMER was used.
 * 
 * The main idea of the approach described is that the TimerFrame variable will be destroyed
 * at the end of the block where START_TIMER macro was used. In order to work properly
 * in situations where END_TIMER was used to stop the timer manually before (but there is still the
 * variable which will be later destroyed), we have to store references to these variables and
 * destroy them on-demand.
 */
class TimerFrame {
private:
    string tag;
    TimerFrame* _parent;
    bool closed;
    static map<string, TimerFrame*> _frames;
public:

    /**
     * Parent of the TimerFrame object (it is a TimerFrame object with the same tag,
     * but defined in the superior block of code or function)
     */
    TimerFrame* parent() {
        return _parent;
    }

    TimerFrame(string tag);

    ~TimerFrame();

    /**
     * If not already closed, closes the TimerFrame object.
     * Asks Prifler to end a timer with specified tag and changes the frames
     * map appropriately (if the TimerFrame object has a parent, associate hits parent
     * with the tag or if not, delete the tag from the map)
     */
    void close();

    /**
     * Stops the timer manually
     * @param tag - timer name
     */
    static void endTimer(string tag);

    /**
     * Tags with associated TimerFrame objects
     */
    static map<string, TimerFrame*>* frames() {
        return &_frames;
    }
};

#endif