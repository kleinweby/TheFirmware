#!/usr/bin/env ruby

require_relative './BuildSupport/Configure'

Configure.new(File.expand_path(File.dirname(__FILE__))).run()
