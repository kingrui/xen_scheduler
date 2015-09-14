#!/usr/bin/env ruby

module Perf
    SEP = ':'
    def self.parse_stat(filename)
        hash = Hash.new
        File.open(filename, 'r') do |f|
            lines = f.readlines
            lines.each do |line|
                v = line.split SEP
                hash[v[1].strip] = v[0].to_f
            end
        end
        hash
    end

    def self.cpi(hash)
        hash['cycles'] / hash['instructions']
    end

    def self.cache_miss_rate(hash)
        hash['cache-misses'] / hash['cache-references']
    end
    
    def self.mpki(hash)
        hash['cache-misses'] / hash['instructions'] * 1000
    end

    def self.branch_miss_rate(hash)
        hash['branch-misses'] / hash['branches']
    end

end

if __FILE__ == $0
    puts Perf.cpi Perf.parse_stat ARGV[0]
end
