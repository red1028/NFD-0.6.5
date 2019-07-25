# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Options

def addIbrdtnOptions(self, opt):
    opt.add_option('--with-ibrdtn', action='store_true', default=False,
                   dest='enable_ibrdtn', help='''Enable IBR-DTN support''')
setattr(Options.OptionsContext, "addIbrdtnOptions", addIbrdtnOptions)

def configure(conf):
    #def boost_asio_has_local_sockets():
    #    return conf.check_cxx(msg='Checking if Unix sockets are supported',
    #                          fragment=BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK,
    #                          features='cxx', use='BOOST', mandatory=False)

    #if conf.options.force_unix_socket or boost_asio_has_local_sockets():
    conf.define('HAVE_IBRDTN', 1)
    conf.env['HAVE_IBRDTN'] = True
