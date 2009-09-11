# Software License Agreement (BSD License)
#
# Copyright (c) 2008, Willow Garage, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of Willow Garage, Inc. nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Revision $Id$

import os
import logging
import sys
import time
import traceback

import roslib.packages
import roslib.roslogging
import roslib.scriptutil

import roslaunch.core

# symbol exports
from roslaunch.core import Node, Test, Master, RLException
from roslaunch.config import ROSLaunchConfig
from roslaunch.launch import ROSLaunchRunner
from roslaunch.xmlloader import XmlLoader, XmlParseException

NAME = 'roslaunch'

## @param options_runid str: run_id value from command-line or None
## @param options_wait_for_master bool: the wait_for_master command
## option. If this is True, it means that we must retrieve the value
## from the parameter server and need to avoid any race conditions
## with the roscore being initialized.
def _get_or_generate_uuid(options_runid, options_wait_for_master):
    # Three possible sources of the run_id:
    #
    #  - if we're a child process, we get it from options_runid
    #  - if there's already a roscore running, read from the param server
    #  - generate one if we're running the roscore
    if options_runid:
        return options_runid

    # #773: Generate a run_id to use if we launch a master
    # process.  If a master is already running, we'll get the
    # run_id from it instead
    param_server = roslib.scriptutil.get_param_server()
    val = None
    while val is None:
        try:
            code, msg, val = param_server.getParam('/roslaunch', '/run_id')
            if code == 1:
                return val
            else:
                raise RuntimeError("unknown error communicating with Parameter Server: %s"%msg)
        except:
            if not options_wait_for_master:
                val = roslaunch.core.generate_run_id()
    return val
    
## scripts using roslaunch MUST call configure_logging
def configure_logging(uuid):
    try:
        import socket
        logfile_basename = os.path.join(uuid, '%s-%s-%s.log'%(NAME, socket.gethostname(), os.getpid()))
        # additional: names of python packages we depend on that may also be logging
        logfile_name = roslib.roslogging.configure_logging(NAME, filename=logfile_basename, additional=['paramiko', 'roslib', 'rospy'])
        if logfile_name:
            print "... logging to %s"%logfile_name

        # add logger to internal roslaunch logging infrastructure
        logger = logging.getLogger('roslaunch')
        roslaunch.core.add_printlog_handler(logger.info)
        roslaunch.core.add_printerrlog_handler(logger.error)
    except:
        print >> sys.stderr, "WARNING: unable to configure logging. No log files will be generated"
        
## block until master detected
def _wait_for_master():
    m = roslaunch.core.Master() # get a handle to the default master
    is_running = m.is_running()
    if not is_running:
        roslaunch.core.printlog("roscore/master is not yet running, will wait for it to start")
    while not is_running:
        time.sleep(0.1)
        is_running = m.is_running()
    if is_running:
        roslaunch.core.printlog("master has started, initiating launch")
    else:
        raise RuntimeError("unknown error waiting for master to start")

def main(argv=sys.argv):
    try:
        from optparse import OptionParser

        parser = OptionParser(usage="usage: %prog [-p|--port=port] [--core] [-u|--server_uri=uri]  [-c|--child_name=name] [files...]", prog=NAME)
        parser.add_option("--args",
                          dest="node_args", default=None,
                          help="Print command-line arguments for node", metavar="NODE_NAME")
        parser.add_option("-c", "--child",
                          dest="child_name", default=None,
                          help="Run as child service 'NAME'. Required with -u", metavar="NAME")
        parser.add_option("--local",
                          dest="local_only", default=False, action="store_true",
                          help="Do not launch remote nodes")
        parser.add_option("-u", "--server_uri",
                          dest="server_uri", default=None,
                          help="URI of server. Required with -c", metavar="URI")
        parser.add_option("--run_id",
                          dest="run_id", default=None,
                          help="run_id of session. Required with -c", metavar="RUN_ID")
        # #1254: wait until master comes online before starting
        parser.add_option("--wait", action="store_true",
                          dest="wait_for_master", default=False,
                          help="wait for master to start before launching")
        parser.add_option("-p", "--port",
                          dest="port", default=None,
                          help="master port. Only valid if master is launched", metavar="PORT")
        parser.add_option("--core", action="store_true",
                          dest="core", default=False, 
                          help="Launch core services only")

        (options, args) = parser.parse_args(argv[1:])

        # validate args first so we don't spin up any resources
        if options.child_name:
            if not options.server_uri:
                parser.error("--child option requires --server_uri to be set as well")
            if not options.run_id:
                parser.error("--child option requires --run_id to be set as well")                
            if options.port:
                parser.error("port option cannot be used with roslaunch child mode")
            if args:
                parser.error("Input files are not allowed when run in child mode")
        elif options.core:
            if args:
                parser.error("Input files are not allowed when launching core")
            if options.run_id:
                parser.error("--run_id should only be set for child roslaunches (-c)")
                
            # we don't actually do anything special for core as the roscore.xml file
            # is an implicit include for any roslaunch

        elif len(args) == 0:
            parser.error("you must specify at least one input file")
        elif [f for f in args if not os.path.exists(f)]:
            parser.error("The following input files do not exist: %s"%f)


        # node args doesn't require any roslaunch infrastructure, so process it first
        if options.node_args:
            if not args:
                parser.error("please specify a launch file")
            import roslaunch.node_args
            roslaunch.node_args.print_node_args(options.node_args, args)
            return

        # we have to wait for the master here because we don't have the run_id yet
        if options.wait_for_master:
            if options.core:
                parser.error("--wait cannot be used with roscore")
            _wait_for_master()            

        # spin up the logging infrastructure. have to wait until we can read options.run_id
        uuid = _get_or_generate_uuid(options.run_id, options.wait_for_master)
        configure_logging(uuid)

        logger = logging.getLogger('roslaunch')
        logger.info("roslaunch starting with args %s"%str(argv))
        logger.info("roslaunch env is %s"%os.environ)
        #logger.info("roslaunch os.login: %s"%os.getlogin())
            
        if options.child_name:
            logger.info('starting in child mode')

            # This is a roslaunch child, spin up client server.
            # client spins up an XML-RPC server that waits for
            # commands and configuration from the server.
            import child
            c = child.ROSLaunchChild(uuid, options.child_name, options.server_uri)
            c.run()
        else:
            logger.info('starting in server mode')

            # This is a roslaunch parent, spin up parent server and launch processes.
            # args are the roslaunch files to load
            import parent
            p = parent.ROSLaunchParent(uuid, args, is_core=options.core, port=options.port, local_only=options.local_only)
            p.start()
            p.spin()

    except RLException, e:
        roslaunch.core.printerrlog(str(e))
        sys.exit(1)
    except Exception, e:
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
