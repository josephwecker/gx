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
             vs     = '0'
             vv     = 'NULL'
             if ckey == 'type'
               vv   = '"unknown"'
               vs   = '0x%04x' % (vv.size - 2)
             elsif ckey == 'severity'
               vv   = '"SEV_UNKNOWN"'
               vs   = '0x%04x' % (vv.size - 2)
             end
             ['0', '0x%04x' % (ckey.size), '"'+ckey+'"', vs, vv]
           end
ah_offs  = keys.size
keys    += [['0','0','NULL','0','NULL']]*ADHOCS
sz       = keys.size

puts <<-OUTPUT
    static _GX_KV_SIZETYPE _key_sizes_master[] = {#{keys.map{|k| k[1]}.join(',')}};
    static _GX_KV_SIZETYPE _val_sizes_master[] = {#{keys.map{|k| k[3]}.join(',')}};
    static _GX_KV_SIZETYPE _key_sizes[#{sz}];
    static _GX_KV_SIZETYPE _val_sizes[#{sz}];

    static const _gx_kv msg_tab_master[] = {
        #{keys.each_with_index.map{|k,i| "_make_gx_kv(#{i},#{k[1..-1].join(',')})"}.join(",\n        ")}
    };

    static _gx_kv msg_tab[#{sz}];
    #define adhoc_offset #{ah_offs}
    #define adhoc_last   #{sz - 1}
OUTPUT
