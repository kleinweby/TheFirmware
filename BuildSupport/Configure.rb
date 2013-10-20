require_relative 'Ninja'
require 'optparse'
require 'ostruct'
require 'pathname'
require 'time'

class Late

  def initialize(&block)
    @block = block
  end

  def eval
    @block.call
  end

end

class BuildConfiguration < Ninja

  @@CurrentStack = []

  def self.current
    @@CurrentStack.last
  end

  def self.push(config)
    @@CurrentStack.push(config)
  end

  def self.pop
    @@CurrentStack.pop
  end

  def initialize(parent)
    super(parent)

    @ext_rules = {}
    self._objects = []
  end

  attr_accessor :_objects

  # Conveinent methods
  def file(glob, extra_options = {}, &block)
    Pathname.glob(glob) do |file|
      # Re-relativ path
      file = path(file)
      obj = path(file.join(self.root.options.builddir, file.to_s + '.o'))
      rulename = extra_options[:rulename] || rule_for_file(file)

      build obj, rulename, file
      self._objects << obj
    end
  end

  def rule_for_file(file)
    ext = File.extname(file)

    if @ext_rules.has_key? ext
      @ext_rules[ext]
    elsif not self.parent.nil?
      self.parent.rule_for_file(file)
    else
      raise "Could not infer rule for #{ext}"
    end
  end

  def register_default_rule(ext, rulename)
    @ext_rules[ext] = rulename
  end

  def root
    return nil if self.parent.nil?

    parent = self.parent

    while not parent.parent.nil?
      parent = parent.parent
    end

    parent
  end

  def objects
    conf = self
    Late.new do 
      conf._objects
    end
  end

  def path(path)
    path = Pathname.new(path) unless path.is_a? Pathname

    path.expand_path.relative_path_from(Pathname.new(self.root.options.basedir))
  end
end

class Target < BuildConfiguration
  attr_accessor :name
  attr_accessor :base
  attr_accessor :abstract

  def initialize(parent, name, args)
    super(parent)

    self.name = name
    self.base = args[:base]
    self.abstract = args[:abstract] || false
  end
end

class Board < BuildConfiguration
  attr_accessor :name
  attr_accessor :_targets

  def initialize(parent, name)
    super(parent)

    self.name = name
    self._targets = []
  end

  def targets(targets)
    if targets.is_a? Enumerable
      self._targets += targets
    else
      self._targets << targets
    end
  end
end

class Configure < BuildConfiguration
  attr_accessor :options

  def initialize(default_basedir)
    super(nil)

    self.options = OpenStruct.new
    self.options.basedir = default_basedir
    self.options.action = :write
    self.options.debug = true
    @configre_deps = [File.expand_path(__FILE__), File.expand_path(File.join(File.dirname(__FILE__), 'Ninja.rb'))]
    @targets = {}
    @boards = {}
  end

  def run
    var :configure_args, ARGV.join(" ")
    self.parse_args ARGV
    var :builddir, self.options.builddir

    BuildConfiguration.push(self)
    self.parse_buildfile(File.join(self.options.basedir, 'BuildSupport', 'Rules.rb'))
    self.parse_buildfile(File.join(self.options.basedir, 'Firmware', 'Build'))
    self.parse(File.join(self.options.basedir, 'Boards', '*', 'Build'))
    BuildConfiguration.pop

    case self.options.action
    when :write
      self.choose_board
      self.choose_target
      self.write_build
    when :list_targets
      puts("Avaiable targets:")

      @targets.each do |n,t|
        unless t.abstract
          puts("  - #{n}")
        end
      end
    when :list_boards
      puts("Avaiable boards:")

      @boards.each do |n,t|
        puts("  - #{n}")
      end
    end
  end

  def parse(glob)
    Dir.glob(glob) do |file|
      self.parse_buildfile(File.expand_path(file))
    end
  end

  def target(name, args = {}, &block)
    t = Target.new(self, name, args)

    BuildConfiguration.push(self)
    block.arity < 1 ? t.instance_eval(&block) : block.call(t) if block_given?
    BuildConfiguration.pop

    raise "Target #{t.name} was already defined." if @targets.has_key? t.name.downcase

    @targets[t.name.downcase] = t
  end

  def board(name, &block)
    b = Board.new(self, name)

    BuildConfiguration.push(self)
    block.arity < 1 ? b.instance_eval(&block) : block.call(b) if block_given?
    BuildConfiguration.pop

    raise "Board #{name} was already defined." if @boards.has_key? b.name.downcase

    @boards[b.name.downcase] = b
  end

  @private
  def parse_args(args)
    opt_parser = OptionParser.new do |opts|

      opts.on('--basedir BASEDIR', "The dir that contains TheFirmware") do |basedir|
        self.options.basedir = basedir
      end

      opts.on('-b', '--board BOARD', 'The board to compile for.') do |board|
        if board == 'list'
          self.options.action = :list_boards
        else
          self.options.board = board
        end
      end

      opts.on('-t', '--target TARGET', 'The target to compile for.') do |target|
        if target == 'list'
          self.options.action = :list_targets
        else
          self.options.target = target
        end
      end

      opts.on('--disable-debug', 'Disable debug and enables optimazions') do
        self.options.debug = false
      end

      opts.on_tail("--extra-env-paths") do
        puts(File.join(self.options.basedir, 'Toolchain', 'arm.toolchain', 'bin') + ':' + File.join(self.options.basedir, 'Toolchain', 'arm-gcc.toolchain', 'bin'))
        exit
      end

      opts.on_tail("-h", "--help", "Show this message") do
        puts opts
        exit
      end
    end

    opt_parser.parse!(args)
    self.options.builddir = File.join(self.options.basedir, '.build')
  end

  def parse_buildfile(file)
    Dir.chdir(File.dirname(file)) do 
      load file, wrap = true
    end

    @configre_deps << file
  end

  def to_ninja(indention = 0)
    str  = "################################################################################\n"
    str += "# TheFirmware                                                                  #\n"
    str += "#                                                                              #\n"
    str += "# " + "generated by configure.rb on #{Time.now.asctime}".ljust(76) + " #\n"
    str += "# DO NOT EDIT THIS FILE.                                                       #\n"
    str += "################################################################################\n\n"
    
    str + super
  end

  def write_build
    # Declare which files will cause a reconfigure
    build 'build.ninja', 'configure', [], @configre_deps.map {|f| path(f) }

    File.open('build.ninja', 'w') do |f|
      f.write(self.to_ninja)
    end
  end

  def root
    self
  end

  def choose_board
    raise "Unkown board #{self.options.board}" unless @boards.has_key? self.options.board.downcase

    board = @boards[self.options.board.downcase]

    if self.options.target.nil?
      raise "Please specify a target, this board supports: #{board._targets.join(", ")}" unless board._targets.count == 1

      self.options.target = board._targets.first
    elsif not board._targets.map {|x| x.downcase}.include? self.options.target.downcase
      raise "Board does not support requested target"
    end

    self._objects += board._objects
    self.defs << board
    var :define, "-DBOARD=#{board.name.upcase}"
  end

  def choose_target
    raise "You need to specify a target" if self.options.target.nil?
    raise "Unkown target #{self.options.target}" unless @targets.has_key? self.options.target.downcase

    additional = []
    t = @targets[self.options.target.downcase]

    var :define, "-DTARGET=#{t.name.upcase}"
    while not t.nil?
      additional.unshift(t)
      self._objects += t._objects
      var :define, "-DTARGET_#{t.name.upcase}=1"

      if t.base.nil?
        break
      end

      raise "Target #{t.name} is based on unkown target #{t.base}" unless @targets.has_key? t.base.downcase
      t = @targets[t.base.downcase]
    end

    self.defs += additional
  end
end

def method_missing name, *args, &block
  if self.class == Object and BuildConfiguration.current.respond_to? name
    BuildConfiguration.current.send(name, *args, &block)
  else
    super
  end
end
