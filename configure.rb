#!/usr/bin/env ruby

require_relative './tools/configure/Configure'

Configure.new(File.expand_path(File.dirname(__FILE__))).run()
