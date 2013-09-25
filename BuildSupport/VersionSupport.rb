file 'Firmware/FirmwareInfo.cc' => [ 'Firmware/FirmwareInfo.cc.rake-defs' ] do |t|
  defs = {}
  open('Firmware/FirmwareInfo.cc.rake-defs') do |f|
    defs = JSON.load(f)
  end
  
  content = <<-eos
namespace TheFirmware {
const char* FirmwareVersion = "#{defs['version']}";
const char* FirmwareGitVersion = "#{defs['gitVersion'] || "NULL"}";
const char* FirmwareGitBranch = "#{defs['gitBranch'] || "NULL"}";
const char* FirmwareBuildDate = "#{defs['date']}";
}
  eos
  
  open(t.name, "w") do |f|
    f.write(content)
  end
end

# A KernelInfo Task
class FirmwareVersionDefTask < Rake::FileTask
  @_currentDefs
  
  def initialize(task_name, app)
    super(task_name, app)
    @actions << Proc.new do |t|
      
      open(name, "w") do |f|
        JSON.dump(currentDefs(), f)
      end
    end
  end
  
  def out_of_date?(stmp)
    defs = {}
    begin
      open(name) do |f|
        defs = JSON.load(f)
      end if File.file?(name)
    rescue
      defs = {}
    end
    
    defs != currentDefs()
  end
  
  def currentDefs
    @_currentDefs ||= {
      'gitVersion' => `git rev-parse --short HEAD`.strip,
      'gitBranch' => `git rev-parse --abbrev-ref HEAD`.strip,
      'date' =>  Time.new.to_s,
      'version' => `git describe --match 'version/*' --always --dirty`.strip().sub('version/', '')
    } 
    @_currentDefs
  end
end

FirmwareVersionDefTask.define_task 'Firmware/FirmwareInfo.cc.rake-defs' => []