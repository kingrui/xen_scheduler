#!/usr/bin/env ruby

require 'erb'

class Matfig

  def initialize(out)
    @out = out
  end

  def self.gen(out, &block)
    Matfig.new(out).instance_eval(&block)
  end

  def function(name)
    @out.puts "function #{name}"
  end

  def method_missing(name, *args)
    @out.puts "#{name}(#{args.join(',')});"
  end

  def self.clean(name)
    name.tr('^A-Za-z0-9', '')
  end

  def self.plot(x, y, pstyle, plabel, xlabel, ylabel, mname, outdir)
    mname = clean mname
    File.open("#{outdir}/#{mname}.m", "w") do |f|
      f.puts((ERB.new IO.read '../templates/plot.erb').result(binding))
    end
    system("cd #{outdir}\nmatlab -nodisplay -nosplash -nodesktop -r #{mname}")
  end

  def self.bar(data, xtics, xlabel, ylabel, mname, outdir, mse=nil) 
    mname = clean mname
    File.open("#{outdir}/#{mname}.m", "w") do |f|
      f.puts((ERB.new IO.read '../templates/bar.erb').result(binding))
    end
    system("cd #{outdir}\nmatlab -nodisplay -nosplash -nodesktop -r #{mname}")
  end

end
