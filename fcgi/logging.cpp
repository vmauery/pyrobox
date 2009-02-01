/**************************************************************************************************
 *     CVS $Id: logging.cpp 380 2003-10-10 20:19:32Z vhmauery $
 * DESCRIPTION: Utilities for Logging
 *     AUTHORS: Marc Strämke, Darren Hart
 *  START DATE: 2003/Jun/28
 *
 *   COPYRIGHT: 2003 by Darren Hart, Vernon Mauery, Marc Strämke, Dirk Hörner
 *     LICENSE: This software is licenced under the Libstk license available with the source as 
 *              license.txt or at http://www.libstk.org/index.php?page=docs/license
 *************************************************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include "logging.hpp"

namespace Fastcgipp
{
    boost::shared_ptr<logger> logger::instance_;
    
    boost::shared_ptr<logger> logger::get()
    {
        if(!instance_)
            instance_.reset(new logger());
        return instance_;
    }

	void logger::shutdown() {
		instance_.reset();
	}
    
    logger::logger()
    {
        severity_names_.resize(LL_LENGTH);
        severity_names_[LL_Info] = "Info";
        severity_names_[LL_Warning] = "Warning";
        severity_names_[LL_Error] = "Error";
        severity_names_[LL_None] = "None";
    }

    logger::~logger()
    {
        log(__FILE__, __LINE__, std::string("destructor"), LL_Info);
    }
    
    void logger::add_target(const char *target, log_level min_level)
    {
        target_info temp;
		temp.name = target;
		std::ofstream *os = new std::ofstream(target, std::ios_base::out | std::ios_base::app);
        temp.outstream = os;
        temp.min_level = min_level;
        targets.push_back(temp);
		temp.name = NULL;
    }
    
    void logger::add_target(std::ostream* target, log_level min_level)
    {
        target_info temp;
        temp.outstream = target;
        temp.min_level = min_level;
        targets.push_back(temp);
    }
    
    void logger::remove_target(const std::string& target)
    {
        Ttargets::iterator iter = std::find(targets.begin(), targets.end(), target);
        targets.erase(iter);
    }
    
    void logger::remove_target(std::ostream* target)
    {
        Ttargets::iterator iter = std::find(targets.begin(), targets.end(), target);
        targets.erase(iter);
    }
    
    void logger::log(const std::string& filename, int line, const std::string& message, 
            log_level severity)
    {
        for (Ttargets::iterator iter=targets.begin();iter!=targets.end();iter++)
        {
            if(severity >= iter->min_level)
                *iter->outstream << severity_names_[severity] << "! " << filename << ":" << line 
                                 << " \t" << message << std::endl;
				iter->outstream->flush();
        }
        
    }
    
    void logger::write(const std::string& filename, int line, const char *msg, size_t len,
            log_level severity)
    {
        for (Ttargets::iterator iter=targets.begin();iter!=targets.end();iter++)
        {
            if(severity >= iter->min_level)
                *iter->outstream << severity_names_[severity] << "! " << filename << ":" << line << std::endl;
				iter->outstream->write(msg, len);
				*iter->outstream << std::endl;
				iter->outstream->flush();
        }
        
    }
    
    
}
