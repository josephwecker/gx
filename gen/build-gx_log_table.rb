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
             vs     = '0x0003'
             vv     = '_GX_NULLSTRING'
             if ckey == 'type'
               vv   = '"unknown"'
               vs   = '0x%04x' % (vv.size+3 - 2)
             elsif ckey == 'severity'
               vv   = '"SEV_UNKNOWN"'
               vs   = '0x%04x' % (vv.size+3 - 2)
             end
             ['0', '0x%04x' % (ckey.size+3), '"'+ckey+'"', vs, vv]
           end
ah_offs  = keys.size
keys    += [['0','0x0003','_GX_NULLSTRING','0x0003','_GX_NULLSTRING']]*ADHOCS
sz       = keys.size

puts <<-OUTPUT
static const _gx_kv msg_tab_master[] = {
    #{keys.map{|k|"{#{k.join(',')}}"}.join(",\n    ")}
};
static _gx_kv msg_tab[#{sz}];
#define adhoc_offset #{ah_offs}
#define adhoc_last   #{sz - 1}
OUTPUT
