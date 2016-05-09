/*!
 *
﻿ * Copyright (C) 2015 Technical University of Liberec.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation. (http://www.gnu.org/licenses/gpl-3.0.en.html)
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *
 * @file    logger.hh
 * @brief
 */

#ifndef LOGGER_HH_
#define LOGGER_HH_


#include <iostream>
#include <sstream>
#include <fstream>



/**
 * Helper class defined logger file.
 *
 * Use singleton design pattern.
 */
class LoggerFileStream : public std::ofstream
{
public:
    /// Getter of singleton instance object
	static LoggerFileStream& get_instance();

    /// Returns number of actual process, if MPI is not supported returns 0.
	static int get_mpi_rank();

    /// Initialize instance object in format 'log_file_name.process.log'
	static void init(const std::string &log_file_name, bool no_log = false);

    /// Check if singleton instance object is initialize.
	static inline bool is_init()
	{ return (instance_ != nullptr); }

	/// Destructor
	~LoggerFileStream();
private:
	/// Forbidden empty constructor
	LoggerFileStream();

	/// Forbidden constructor
	LoggerFileStream(const char* filename);

	/// Singleton instance
	static LoggerFileStream* instance_;

	/// Turn off logger file output
	static bool no_log_;

	friend class MultiTargetBuf;
};


/**
 * Helper class
 *
 * Manage logger stream that is used for all logger outputs.
 */
class MultiTargetBuf : public std::stringbuf
{
public:
	/// Enum of types of Logger messages.
	enum MsgType {
		warning, message, log, debug
	};

	/// Return string value of given MsgType
	static const std::string msg_type_string(MsgType msg_type);

	/// Constructor
	MultiTargetBuf(MsgType type);

	/// Stores values for printing out line number, function, etc
	void set_context(const char* file_name, const char* function, const int line);

	/// Set flag every_process_ to true
	void every_proc();
protected:
	virtual int sync();
private:
	static const unsigned int mask_cout = 0b00000001;
	static const unsigned int mask_cerr = 0b00000010;
	static const unsigned int mask_file = 0b00000100;

	/// Set @p streams_mask_ according to the tzpe of message.
	void set_mask();

	/// Print formated message to given stream if mask corresponds with @p streams_mask_.
	void print_to_stream(std::ostream& stream, unsigned int mask);

	MsgType type_;                        ///< Type of message.
	bool every_process_;                  ///< Flag marked if log message is printing for all processes or only for zero process.
	std::string file_name_;               ///< Actual file.
	std::string function_;                ///< Actual function.
	int line_;                            ///< Actual line.
	std::string date_time_;               ///< Actual date and time.
	int mpi_rank_;                        ///< Actual process (if MPI is supported)
	int streams_mask_;                    ///< Mask of logger, specifies streams
	bool printed_header_;                 ///< Flag marked message header was printed (in first call of sync method)
	std::ostringstream formated_output_;  ///< Helper stream, store message during printout to individual output
};


/**
 * @brief Class for storing logger messages.
 *
 * Allow define different levels of log messages and distinguish output streams
 * for individual leves. These output streams are -
 *  - standard console output (std::cout)
 *  - standard error output (std::cerr)
 *  - file output (LoggerFileStream class)
 *
 * Logger distinguishes four type of levels -
 *  - warning: printed to standard error and file output
 *  - message: printed to standard console and file output
 *  - log: printed to file output
 *  - debug: printed to file output (level is used only in debug mode)
 *
 * File output is optional. See \p LoggerFileStream for setting this output stream.
 *
 * <b>Example of Logger usage:</b>
 *
 * For individual levels are defined macros -
 *  - MessageOut()
 *  - WarningOut()
 *  - LogOut()
 *  - DebugOut()
 * that ensure display of actual code point (source file, line and function).
 *
 * Logger message is created by using an operator << and allow to add each type
 * that has override this operator. Message is terminated with manipulator
 * std::endl. Implicitly logger message is printed only in processor with rank
 * zero. If necessary printed message for all process, it provides a method
 * every_proc().
 *
 * Examples of logger messages formating:
 *
 @code
   MessageOut() << "End of simulation at time: " << secondary_eq->solved_time() << std::endl;
   WarningOut() << "Unprocessed key '" << key_name << "' in Record '" << rec->type_name() << "'." << std::endl;
   LogOut() << "Write output to output stream: " << this->_base_filename << " for time: " << time << std::endl;
   DebugOut() << "Calling 'initialize' of empty equation '" << typeid(*this).name() << "'." << std::endl;
 @endcode
 *
 * Logger message can be created by more than one separate message (std::endl
 * manipulator can be used multiple times):
 *
 @code
   MessageOut() << "Start time: " << this->start_time() << std::endl << "End time: " << this->end_time() << std::endl;
 @endcode
 *
 * In some cases message can be printed for all processes:
 *
 @code
   MessageOut().every_proc() << "Size distributed at process: " << distr->lsize() << std::endl;
 @endcode
 *
 */
class Logger : public std::ostream {
public:
	/// Constructor.
	Logger(MultiTargetBuf::MsgType type);

	/// Stores values for printing out line number, function, etc
	Logger& set_context(const char* file_name, const char* function, const int line);

	/// Set flag MultiTargetBuf::every_process_ to true
	Logger& every_proc();

	/// Destructor.
	~Logger();

};

/// Internal macro defining universal record of log
#define _LOG(type) \
	Logger( type ).set_context( __FILE__, __func__, __LINE__)
/// Macro defining 'message' record of log
#define MessageOut() \
	_LOG( MultiTargetBuf::MsgType::message ).set_context( __FILE__, __func__, __LINE__)
/// Macro defining 'warning' record of log
#define WarningOut() \
	_LOG( MultiTargetBuf::MsgType::warning ).set_context( __FILE__, __func__, __LINE__)
/// Macro defining 'log' record of log
#define LogOut() \
	_LOG( MultiTargetBuf::MsgType::log ).set_context( __FILE__, __func__, __LINE__)
/// Macro defining 'debug' record of log
#define DebugOut() \
	_LOG( MultiTargetBuf::MsgType::debug ).set_context( __FILE__, __func__, __LINE__)




#endif /* LOGGER_HH_ */
