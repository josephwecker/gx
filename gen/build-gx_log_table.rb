#!/usr/bin/env ruby
# Finds a specific enum in gx_log.h and builds a statically allocated table
# based on it. gx_log.h then includes the generated array and uses it as a
# staging ground as it processes parameters to be logged.
#
# This could be pretty easily rewritten as a shellscript if someone really felt
# the urge. I don't have the patience for it though at the moment.
#
# TODO:
#   - Pull ADHOCS (number of slots for adhoc key-value pairs) from C source
#   - Pull default values from C source
#

ADHOCS   = 40

gx_root  = File.join(File.dirname(__FILE__), '..')
srcf     = "#{gx_root}/gx_log.h"

src      = `gcc -I#{gx_root} -E -P "#{srcf}" 2>/dev/null`      # preprocess
src      = src.gsub(/\s+/m,' ').gsub(/ ?([=,;})({]) /, '\\1')  # condense whitespace
enum_def = src[/\btypedef\s*enum\s*gx_log_standard_keys\s*\{([^\}]+)\}/,1]
abort      "gx_log_standard_keys enum not found in #{srcf}" if enum_def.nil?

keys     = enum_def.split(',').map{|key|
             { :label   => key = key.split('=')[0][/^K_([a-z0-9_]+)$/,1],
               :default => (key == 'type' ? 'unknown' : (key == 'severity' ? 'SEV_UNKNOWN' : nil))}} +
           [ { :label   => nil, :default => nil} ] * ADHOCS
keys     = keys.map do |key|
             if key[:default]
               key[:key_head_size] = key[:val_head_size] = 'sizeof(kv_head_t)'
               key[:key_data_size] = (key[:label].size + 1).to_s
               key[:val_data_size] = (key[:default].size + 1).to_s
             end
             key
           end
sz       = keys.size
ah_offs  = sz - ADHOCS

def ssize(str) "#{(str||'').size.to_s.ljust 3} + 1 + sizeof(kv_head_t)" end # string size + null-terminator + "pkt-length"-header
def quo(str)   "\"#{str.gsub('"','\\"')}\"".ljust(14)                   end
def khs(key)   key[:key_head_size] || '0'                               end
def kds(key)   key[:key_data_size] || '0'                               end
def vhs(key)   key[:val_head_size] || '0'                               end
def vds(key)   key[:val_data_size] || '0'                               end
puts <<-OUTPUT
  #define ADHOC_OFFSET #{ah_offs}
  #define KV_ENTRIES   #{sz}

  static kv_head_t _key_sizes_master[] = {
      #{keys.map{|k| ssize(k[:label]  )}.join(",\n        ")}
  };
  static kv_head_t _val_sizes_master[] = {
      #{keys.map{|k| ssize(k[:default])}.join(",\n        ")}
  };
  static _gx_kv msg_tab_master[] = {
      #{keys.each_with_index.map{|k,i|
          "{&(_key_sizes_master[#{i.to_s.rjust 2}]), #{khs(k)}, #{k[:label]  .nil? ? '_GX_NULLSTRING' : quo(k[:label])}, #{kds(k)}," +
          " &(_val_sizes_master[#{i.to_s.rjust 2}]), #{vhs(k)}, #{k[:default].nil? ? '_GX_NULLSTRING' : quo(k[:default])}, #{vds(k)}}"}.join(",\n        ")}
  };
OUTPUT
