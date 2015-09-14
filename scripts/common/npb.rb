#!/usr/bin/env ruby
require 'set'

module NPB

  NAMES = Set['Class', 'Size', 'Iterations', 'Time in seconds',
              'Total processes', 'Compiled procs', 'Mop/s total',
              'Mop/s/process', 'Operation type', 'Verification',
              'Version', 'Compile date']

  MPI_BENCHS =  Set['cg', 'bt', 'ep', 'ft', 'is', 'lu', 'mg',
                    'sp']

  OMP_BENCHS =  Set['cg', 'bt', 'ep', 'ft', 'is', 'lu', 'mg',
                    'sp']

  MPI_NORMAL_NAME = Set['Time in seconds', 'Mop/s total', 'Mop/s/process']
  OMP_NORMAL_NAME = Set['Time in seconds', 'Mop/s total', 'Mop/s/thread']

  def self.gen_combinations(benchmarks)
    coms = Set.new
    ben_a = benchmarks.to_a
    (2..benchmarks.size/2).each do |n|
      ben_a.combination(n).each do |com|
        coms << com.to_set unless coms.member?((benchmarks - com).to_set)
      end
    end
    coms
  end

  def self.out_name(ben_type, bench, cls, nprocs, other)
    return "#{ben_type}-#{bench}-#{cls}-#{nprocs}-#{other}.txt"
  end

  def self.successful?(info)
    if info['Verification'] and info['Verification']=='SUCCESSFUL'
      return true
    end
    return false
  end

  def self.get_benchs(ben_type)
    case ben_type
    when "mpi" then NPB::MPI_BENCHS
    when "omp" then NPB::OMP_BENCHS
    end
  end

  def self.get_names(ben_type)
    case ben_type
    when "mpi" then NPB::MPI_NORMAL_NAME
    when "omp" then NPB::OMP_NORMAL_NAME
    end
  end

  def self.parse(fname, names)
    file = File.new fname
    lines = file.readlines
    info = Hash.new 'UNKNOWN'
    lines.each do |line|
      names.each do |name|
        if line.strip.index(name) == 0 and line.index('=')
          info[name] = line.split('=')[1].strip
        end
      end
    end
    return info
  end

  def self.getOutFileName(dir, ben_type, bench, cls, nprocs, other)
    return File.join(dir, out_name(ben_type, bench, cls, nprocs, other))
  end

  def self.parseList(ben_type, dir, benchs, cls, nprocs, names, extra)
    info = Hash.new 0
    benchs.each do |bench|
      fname = getOutFileName dir, ben_type, bench, cls, nprocs,
        extra
      tmp_info = parse fname, names
      info[bench] = tmp_info
    end
    return info
  end

  def self.cross_info(ben_type, dir, obench, ibench, omem, imem, ocpu,
                      icpu, cls, nprocs, names, extra)
  fname = getOutFileName dir, ben_type, "#{obench}-#{ibench}", cls,
  nprocs, "mem#{omem}-#{imem}-cpu#{ocpu}-#{icpu}"
  unless File.exist? fname
    fname = getOutFileName dir, ben_type, "#{obench}-#{ibench}",
    cls, nprocs, "mem#{omem}-#{imem}-cpu#{ocpu}-#{icpu}-0"
  end
  return parse fname, names
  end

  def self.get_summary(ben_type, dir, benchs, cls, nprocs, names, extra)
    sum_info = Hash.new nil
    benchs.each do |bench|
      Dir.glob("#{dir}/*") do |f|
        if File.stat(f).directory?
          fname = getOutFileName f, ben_type, bench, cls,
            nprocs, extra
          next unless File.exist?(fname)
          if sum_info[bench]
            sum_info[bench] << (parse fname, names)
          else
            sum_info[bench] = Array.new
            sum_info[bench] << (parse fname, names)
          end
          #(parse fname, names).each { |k, v| puts "#{k}, #{v}" }
        end
      end
    end
    puts sum_info
    sum_info
  end

  def self.tog_info(ben_type, dir, benchs, cls, nprocs, names, extra)
    sum_info = get_summary(ben_type, dir, benchs, cls, nprocs, names, extra)
    avg = Hash.new nil
    mse = Hash.new nil
    sum_info.each do |bench, info_list|
      avg_tmp = Hash.new 0
      mse_tmp = Hash.new 0
      names.each do |name|
        info_list.each do |run_info|
          avg_tmp[name] += run_info[name].to_f
        end
        avg_tmp[name] /= info_list.length
        info_list.each do |run_info|
          e = run_info[name].to_f - avg_tmp[name]
          mse_tmp[name] += e*e
        end
        mse_tmp[name] = Math.sqrt(mse_tmp[name])/info_list.length
      end
      avg[bench] = avg_tmp
      mse[bench] = mse_tmp
    end
    return avg, mse
  end

  def self.parseTable(ben_type, dir, benchs, cls, nprocs, names, extra)
    info = Hash.new(0)
    benchs.each do |bench1|
      benchs.each do |bench2|
        fname = getOutFileName dir, ben_type,
          "#{bench1}-#{bench2}", cls, nprocs, extra
        info["#{bench1}-#{bench2}"] = parse fname, names
        #end
      end
    end
    return info
  end
end

if __FILE__ == $0
  #names = 
  #NPBResultAnalyst::genTable 'mpi', ARGV[0], NPB::MPI_BENCHS,
  #    'A', 1, names
  #NPB::genList 'mpi', ARGV[0], NPB::MPI_BENCHS, 'A', 1,
  #    names
end
