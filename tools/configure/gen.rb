#!/usr/bin/env ruby 

content = <<-eos
namespace TheFirmware {
    const char* FirmwareVersion = "#{`git describe --match 'version/*' --always --dirty`.strip().sub('version/', '')}";
    const char* FirmwareGitVersion = "#{`git rev-parse --short HEAD`.strip || "NULL"}";
    const char* FirmwareGitBranch = "#{`git rev-parse --abbrev-ref HEAD`.strip || "NULL"}";
    const char* FirmwareBuildDate = "#{Time.new.to_s}";
}
eos

File.open(ARGV[0], 'w') do |f|
	f.write content
end