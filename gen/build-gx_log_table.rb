#!/usr/bin/env ruby
# Finds a specific enum in gx_log.h and builds a statically allocated table
# based on it. gx_log.h then includes the generated array and uses it as a
# staging ground as it processes parameters to be logged.
#
# This could be pretty easily rewritten as a shellscript if someone really felt
# the urge. I don't have the patience for it though at the moment.
#
ADHOCS   = 40

gx_root  = File.join(File.dirname(__FILE__), '..')
srcf     = "#{gx_root}/gx_log.h"

src      = `gcc -I#{gx_root} -E -P "#{srcf}" 2>/dev/null`      # preprocess
src      = src.gsub(/\s+/m,' ').gsub(/ ?([=,;})({]) /, '\\1')  # condense whitespace
enum_def = src[/\btypedef\s*enum\s*gx_log_standard_keys\s*\{([^\}]+)\}/,1]
abort      "gx_log_standard_keys enum not found in #{srcf}" if enum_def.nil?

keys     = enum_def.split(',').map do |key|
             key    = key.split('=')[0]
             ckey   = key[/^K_([a-z0-9_]+)$/,1]
             abort    "unrecognized key: #{key}" unless ckey
             ['0', '0x%04x' % (ckey.size+3), '"'+ckey+'"', '0x0003', '_GX_NULLSTRING']
             #'        {0, 0x%04x, %-20s 0x0003, _GX_NULLSTRING}' % [ckey.size+3,'"'+ckey+'",']
           end

keys    += [['0','0x0003','_GX_NULLSTRING','0x0003','_GX_NULLSTRING']]*ADHOCS
fullsize = keys.size

require 'pp'
pp keys
exit

puts <<-OUTPUT
typedef struct _gx_log_kv_entry {
    char       include [#{fullsize}];
    uint16_t   key_size[#{fullsize}];  ///< Including null terminator & two byte size
    char      *key     [#{fullsize}];
    uint16_t   val_size[#{fullsize}];
    char      *val     [#{fullsize}];
} _gx_log_kv_entry;

static _gx_log_kv_entry log_staging_table = {
    .include   = {0},
    .key_size  = 

puts       keys.join(",\n") + ",\n"
puts       "        {0, 0x0003, _GX_NULLSTRING,      0x0003, _GX_NULLSTRING},\n" * ADHOCS
puts       "    };\n"

puts       "    int adhoc_offset = #{keys.size};\n"
puts       "    int adhoc_last   = #{ADHOCS + keys.size - 1};\n"
puts       "    #define LOG_STAGING_TABLE_ENTRIES #{ADHOCS + keys.size}\n"

OUTPUT
