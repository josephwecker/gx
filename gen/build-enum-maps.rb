#!/usr/bin/env ruby
unless Kernel.respond_to?(:require_relative)
  module Kernel
    def require_relative(path) require File.join(File.dirname(caller[0]), path.to_str) end
  end
end
require          'pp'
require          'tempfile'
require_relative 'perfect_map'
include          PerfectMap

# Find all relevant files where enums might exist
files    = []
inc_dirs = {}
ARGV.each do |arg|
  if arg[-2..-1] == '.h'
    files << arg
    inc_dirs[File.dirname(arg)] = true
    inc_dirs[File.dirname(arg)+'/..'] = true
  elsif arg !~ /\.[a-z]+$/
    inc_dirs[arg] = true
  end
end
inc_dirs = inc_dirs.keys

# Run preprocessor on them to resolve dependencies etc. etc.
cmd      = "gcc -I. #{inc_dirs.map{|d| "-I'#{d}'"}.join(' ')} -E #{files.join(' ')}"
raw      = `#{cmd} 2>/dev/null`

# Some quick deletions to make the regexen cleaner
raw      = raw.gsub(/\/\*.*?\*\//m,   '').    # remove block comments
               gsub(/\/\/[^\n]*/m ,   '').    # remove inline comments
               gsub(/\s+/m,           ' ').   # condense whitespace
               gsub(/ ?([=,;})({]) /, '\\1')  # condense whitespace further

clbl     = '[a-zA-Z_][a-zA-Z0-9_]*'
renums   = /\btypedef enum (#{clbl})\{([^\}]+)\}/
rentries = /\b(#{clbl})(?:=([^,]+?))?(?:,|$)/
lists    = raw.scan(renums).map{|n,v| [n,v,v.scan(rentries)]}

# Run them all as small c programs to get the true value as will be seen by the
# output function.
done     = {}

puts "\n\n\n#ifndef NULL\n#define NULL ((void *)0)\n#endif\n\n"

def strsz(str)
  s = [str.size].pack('S')
  '\\x%02x\\x%02x' % [s[0].ord, s[1].ord]
end

lists.each do |name, raw_entries, entries|
  next if done[name] || name == 'rtmp_mtype'

  c_template =<<-ctemplate
      #include <stdio.h>
      typedef enum #{name} {#{raw_entries}} #{name};
      void pb(unsigned char *v, size_t len) {
          int i;
          for(i=0; i<len; i++) printf("0x%02x%s",v[i],i+1==len ? "" : ",");
          printf("\\n");
      }
      int main(int argc, char **argv) {
          #{name} val;
          #{entries.map{|e| label = e[0]; "
          printf(\"#{label}|\");
          val = #{label};
          pb((unsigned char *)(&val), sizeof(val));\n"}.join('')}

          return 0;
      }
    ctemplate
    
    actual_vals = ''
    Tempfile.open('bld-enum-maps') do |csrc|
      csrc.write c_template
      csrc.flush
      src_path    = csrc.path
      bin_path    = csrc.path + '__compiled'
      
      compile_cmd = "gcc -x c -o '#{bin_path}' '#{src_path}'"
      compile_res = `#{compile_cmd}`
      actual_vals = `'#{bin_path}'`
      `rm '#{bin_path}'`
    end

    actual_vals = actual_vals.split("\n").map{|v| v.split('|')}
    #actual_vals.map!{|label, vals| [vals.split(',').map{|v| v.hex}, '"'+strsz(label)+label+'"']}
    if name =~ /^kv_log/
      actual_vals.map!{|label, vals| [vals.split(',').map{|v| v.hex}, '"'+label.sub(/^K_/,'')+'"']}
    else
      actual_vals.map!{|label, vals| [vals.split(',').map{|v| v.hex}, '"'+label+'"']}
    end

    header  = "/// Looks up string associated with enums in the #{name} enum table by looking at char buffer.\n"+
              "/// @see $#{name}(int) below for something more useful."

    trailer = "\n\n/// Quick lookup of string for #{name} enum values using integer.\n" +
              "/// @retval NULL if not found.\n"
    trailer+= "static __attribute__((__always_inline__,__pure__)) const char * $#{name}(int val)\n{\n"+
              "    return _#{name}_str_map((const char *)(&val));\n" +
              "}\n"
    puts "\n\n"
    defname = '_' + name.upcase.gsub(/[^A-Z]/,'_')+'_STR_MAP'
    puts "#ifndef #{defname}"
    puts "#define #{defname}"
    puts map_function("_#{name}_str_map", actual_vals, :header=>header, :output_ctype=>'const char *')
    puts trailer
    puts "#endif /* #{defname} */"
    done[name] = true
end
