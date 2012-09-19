#!/usr/bin/env ruby
# A wrapper around gperf that returns a perfect-hash-lookup function for C.
#
# Normalizes input and changes the output as follows:
#  - Assumes length is either embedded in the key buffer or is static
#  - Gains significant speed by allowing for occasional false-matches on unexpected keys.  In other words,
#    you're guaranteed to get the correct value for any key that was in the original expected list, and most
#    of the time you'll get the "nomatch" return value for any keys not in the original list, but occasionally
#    you may get a false match for new keys.
#  - All bundled into a single inlineable function with no preprocessor directives or anything else outside of
#    the function.
#
# TODO:
#  - (d) Comments header
#    - (d) Include the original list of keys- possibly on same line as the return values?
#    - (d) Allow override
#  - (~) Move this to a "lib" subfolder
#  - (~) In failed lookup clause (right before last return statement) -
#        construct an appropriate string possibly w/ the key? (like
#        UNKNOWN_0x231f etc.) (?)
#
require 'pp'   # TODO: remove when done debugging
require 'open3'
require 'set'

class Hash
  def slice!(*keys) replace(slice(*keys)) end
  def slice(*keys)
    allowed = Set.new(respond_to?(:convert_key) ? keys.map { |key| convert_key(key) } : keys)
    hash = {}
    allowed.each { |k| hash[k] = self[k] if has_key?(k) }
    hash
  end
end

module PerfectMap
  attr_accessor :name

  def map_function(name, input_rows, opts=nil)
    raise ArgumentError, "Must specify some input strings" if input_rows.empty?
    pm = PMapBuilder.new(name, opts)
    input_rows.each{|i| pm << i}
    pm.function
  end

  class PMapBuilder
    include Open3
    attr_accessor :inputs

    GPERF_ARGS      = ['--multiple-iterations=200', '--random', '--size-multiple=4', '--language=ANSI-C',
                       '--compare-lengths', '--readonly-tables', '--global-table', '--struct-type',
                       '--omit-struct-type', '--null-strings', '--compare-strncmp']
    GPERF_ARG_NOLEN = '--no-strlen'
    GPERF_CMD       = ['/bin/bash', '-c', '/usr/bin/env gperf '+ GPERF_ARGS.join(' ')]

    GPERF_INPUT =<<-gperf_src
      %{

      %}

      struct result_structure { const char *name; };
      %%
        {KWL}
      %%

    gperf_src

    PM_OPT_DEFAULTS = {
      :output_ctype       => nil,
      :output_nomatch_val => nil,
      :no_header          => nil,
      :header             => nil
    }

    def initialize(name='lookup', opts={})
      @name   = name
      opts  ||= {}
      @opts   = PM_OPT_DEFAULTS.merge(opts.slice(*PM_OPT_DEFAULTS.keys))
      @inputs = []
    end

    def <<(input_val)
      raise ArgumentError,'Input values must be arrays with at least 2 elements (key & at least one return value)' unless input_val.is_a?(Array) && input_val.size > 1
      @inputs << input_val
      @function = @parts = @gperf_out = nil
    end

    def output_ctype
      return @opts[:output_ctype]+' ' if @opts[:output_ctype]
      smpl = @inputs[0]
      if smpl.size == 2
        case smpl[1]
        when String  then 'const char *'
        when Integer then 'int '
        else raise ArgumentError,'Could not determine return value type'
        end
      else "struct #{@name}_result *" end
    end

    def [](k) @parts[k] end

    def output_ctype_clean
      occ = output_ctype.gsub(/\bconst\b|\bstatic\b/,'').strip
      if out_is_pointer? && output_ctype.gsub(/\s+/,'') !~ /(const)?char\*/
        return occ[0..-2].strip
      else return occ end
    end

    def output_nomatch_val
      return @opts[:output_nomatch_val] unless @opts[:output_nomatch_val].nil?
      return @output_nomatch_val unless @output_nomatch_val.nil?
      @output_nomatch_val =
        case output_ctype
        when /\*\s*$/        then "(#{output_ctype.gsub('const ','')})NULL"
        when /unsigned|uint/ then "0"
        when /int/           then "-1"
        else "NULL"
        end
      return @output_nomatch_val
    end

    def iv_for_gperf
      @inputs.map do |iv|
        iv = iv.dup
        if iv.is_a?(Array) then key = iv.shift
        else key=iv; iv=[] end
        out = '"' + iv_val(key) + '"'
        out += ','+iv.join(',') if iv.size > 0
        out
      end
    end

    def iv_val(v)
      case v
      when Array;   v.map{|subv| iv_val(subv)}.join('')
      when Integer; sz=[(v.to_s(16).length/2.0).round,2].max; "\\x%0#{sz}x" % v
      when /0x[0-9a-fA-F]+/; iv_val(v.hex)
      else v
      end
    end

    def gperf_input_doc
      src = GPERF_INPUT.dup
      src.gsub!(/ *\{KWL\}/,  iv_for_gperf.join("\n")) # {KWL} keyword-list
      src.gsub!(/ +%/,'%')  # Unindent
      src
    end

    def gperf_output
      return @gperf_out if @gperf_out
      cmd = GPERF_CMD.dup
      # TODO: Figure out if this is ever _not_ needed
      cmd[-1] += ' ' + GPERF_ARG_NOLEN.dup

      popen3(*cmd) do |gperf_in, gperf_out, gperf_err|
        gperf_in.write(gperf_input_doc)
        gperf_in.close
        err = gperf_err.read.strip; gperf_err.close
        raise IOError, err unless err == ''
        @gperf_out = gperf_out.read; gperf_out.close
      end
      ##--- keep for now --- these are often helpful in debugging the reformatted version
      # $stderr.puts '='*40
      # $stderr.puts @gperf_out
      # $stderr.puts '='*40
      @gperf_out
    end

    def parse_gperf_output
      return @parts if @parts
      raw = gperf_output
      @parts = {}
      @parts[:num_keywords]   = raw[/^#define\s+TOTAL_KEYWORDS\s+(\d+)/, 1].to_i
      @parts[:min_word_len]   = raw[/^#define\s+MIN_WORD_LENGTH\s+(\d+)/,1].to_i
      @parts[:max_word_len]   = raw[/^#define\s+MAX_WORD_LENGTH\s+(\d+)/,1].to_i
      @parts[:min_hash_val]   = raw[/^#define\s+MIN_HASH_VALUE\s+(\d+)/, 1].to_i
      @parts[:max_hash_val]   = raw[/^#define\s+MAX_HASH_VALUE\s+(\d+)/, 1].to_i
      @parts[:max_key_range]  = raw[/\/\* maximum key range = (\d+)/,    1].to_i
      @parts[:num_duplicates] = raw[/, duplicates = (\d+) \*\//,         1].to_i
      hsh                     = raw[/static[a-z ]+\s*hash\s*\([^\)]+\)\s*\{(.*?)\n\}\s+/m,1]
      hsh, @parts[:hash_calc] = hsh.split(/^\s*\};\s*$/)
      if @parts[:hash_calc].nil? && hsh =~ /return/
        @parts[:hash_calc] = hsh
      end
      hshtbl                  = hsh[/static[a-z ]+asso_values\[\]\s*=\s*\{(.+)/m,1]
      @parts[:hash_table]     = hshtbl.strip.gsub(/\n     /,'').gsub(/, /,',').split(',') unless hshtbl.nil?

      #------- Wordlist (final lookup result structure) -------
      lst                     = raw[/result_structure wordlist\[\] =\s*\{(.*?)\n\s*\};\s*\n/m,1]
      lst                     = lst.scan(/\{((""|".*?[^\\]"|[^\}\{]|\{[^\}]*\})+)\}/m)
      qstr                    = /(?:^|,\s*)(".*?[^\\]"|[^,]+)/
      @parts[:wordlist]       = lst.map{|r|r[0].scan(qstr).flatten.map{|v|v.strip!;v=='(char*)0' ? nil : v}}
      keypos                  = raw[/\/*\s*Computed positions:\s*-k'([^']+)'/,1]
      if keypos.nil?
        @parts[:keypositions] = []
      else
        @parts[:keypositions]   = keypos.split(',').map{|v|v.to_i}
      end

      if @parts[:min_word_len] == @parts[:max_word_len] && !@static_width
        @static_width = true
        # rerun! (back when it only added the --no-strlen flag when static_widths==true
      else
        @static_width = false
      end

      calculate_length_val

      @parts
    end

    def calculate_length_val
      if @parts[:hash_calc] =~ /\blen\b/
        # Hash creation relies on the length, which is variable. Going to
        # assume it's part of the payloads and find which position holds a
        # value that we can use to calculate the length.
        min_len       = @inputs.map{|i| i[0].size}.min  # assumes min length < 255 ATM (so byte-at-a-time checks)
        @len_found    = false
        @len_pos      = 0
        @len_offset   = 0
        (0..min_len).each do |pos|
          @len_pos    = pos
          @len_offset = @inputs[0][0][pos].ord - @inputs[0][0].size
          offset_also = true
          @inputs.each do |i|
            curr_off = i[0][pos].ord - i[0].size
            if curr_off != @len_offset
              offset_also = false
              break
            end
          end
          next unless offset_also
          @len_found = true
          break
        end
        raise "None of the bytes in the inputs give a length for variable-length entries" unless @len_found
        @len_offset = - @len_offset
      end
    end


    def f_sig
      # TODO: add hot attribute when using versio of gcc that supports it..?
      f_attributes = ['always_inline', 'nonnull', 'pure']
      f_attr = "__attribute__ (( #{f_attributes.map{|a|"__#{a}__"}.join(', ')} ))"
      f_attr += ' const ' if out_is_pointer? && output_ctype !~ /const/
      "static #{f_attr} #{output_ctype}#{@name.to_s.strip}(register const char *buf)"
    end

    def out_is_struct? () @inputs[0].size > 2              end
    def out_is_pointer?() output_ctype.strip[-1..-1]== '*' end

    def f_out_table
      tname = "static const #{output_ctype_clean} returnvals[]"
      out_entries = @parts[:wordlist].map do |entry|
        if entry == [nil] || entry.nil?
          out_is_struct? ?  "{0}" : output_nomatch_val
        else
          entry = entry.dup
          entry.shift # Don't need the key
          out_is_struct? ? "{#{entry.join(', ')}}" : entry[0].to_s
        end
      end

      last_entry = out_entries[-1]
      out_entries.map!{|e|e.to_s.strip+','}
      out_entries[-1] = last_entry # no comma after last

      mlen = [100,out_entries.map{|e|e.length+1}.max].min
      cols = [(100 - 8) / mlen, 1].max
      out_entries.map!{|e|e.ljust(mlen)} if cols > 1
      oe2  = out_entries.each_slice(cols).map{|cls| cls.join('')}

      return "    #{tname} = {\n#{oe2.map{|e| ' '*8+e}.join("\n")}};\n"
    end

    def f_modified_calculation
      # Fix formatting
      hc = @parts[:hash_calc].strip.
        gsub(/^(\s*)return \s*/,'\1hval = ').
        gsub(/switch\s*\(len\)\s*\{ *\n/m, "switch(len) {\n").
        gsub(/^        ([^ ])/,'            \1').
        gsub(/^      ([^ ])/,'        \1').
        gsub(/^  ([^ ])/,'    \1').
        gsub(/^        \/\*/,' '*12+'/*').
        gsub(/^([^ ])/,'    \1').
        gsub(/\bstr\b/,'buf')

      if hc !~ /^\s*(register)?\s*int hval\b/
        hc = "    register unsigned int hval;\n\n" + hc
      else
        hc.gsub!('int hval', 'unsigned int hval')
      end

      # TODO: Fix return value to not be an address if the type is an integer or something
      if @len_found
        lendec = "    register unsigned int len  = ((unsigned char *)buf)[#{@len_pos}]"
        if    @len_offset < 0 then lendec += " - #{@len_offset.abs}"
        elsif @len_offset > 0 then lendec += " + #{@len_offset}" end
        hc = lendec + ";\n" + hc
      end
      hc
    end

    def f_returnval
      if out_is_struct? then '&(returnvals[hval])'
      else 'returnvals[hval]' end
    end

    def minimal_hash?
      parse_gperf_output
      @parts[:num_keywords] == @parts[:wordlist].size
    end

    def comment_details
      det = @parts.slice(:num_keywords, :min_word_len, :max_word_len, :keypositions)
      det[:num_trained_inputs] = det.delete(:num_keywords)
      det[:min_input_size] = det.delete(:min_word_len)
      det[:max_input_size] = det.delete(:max_word_len)
      det[:critical_positions] = det.delete(:keypositions).join(', ') +"\n*"+'  (False positives can occur if rogue input has these positions in common w/ an expected input)'
      det.map{|k,v| "* - #{(k.to_s + ':').ljust(20)} #{v}"}.join("\n")
    end

    def f_header_comment
      return '' if @opts[:no_header]
      return @opts[:header] if @opts[:header]
      headerc = <<-headc
                /**
         * #{@name} is a perfect #{minimal_hash? ? "minimal" : "(almost minimal)"} hash lookup function generated for speed.
         *
         * @param   buf                       Buffer pointing to the "key" that will look up a return value
         *
         * @retval  #{output_nomatch_val.to_s.gsub(/\s+|\([^\)]*\)/,'').ljust(25)} Indicates key didn't have a value#{out_is_struct? && !minimal_hash? ? "
         * @retval  &{0}                      Also indicates nothing matched" : ''}
         * @retval  #{output_ctype.gsub(/\s+/,'').ljust(25)} The value for the given key (see below for caveats though)
         *
         * This function was automatically generated by PerfectMap- it would probably be unwise to
         * change directly. It was generated with a wrapper around gperf that changes the output as
         * follows:
         *
         *  - Assumes length is either embedded in the key buffer or is static
         *  - Gains significant speed by allowing for occasional false-matches on unexpected keys.
         *    **In other words, you're guaranteed to get the correct value for any key that was in the
         *    original expected list, and most of the time you'll get the "nomatch" return value for
         *    any keys not in the original list, but occasionally you may get a false match for new
         *    keys.**
         *  - All bundled into a single inlineable function with no preprocessor directives or
         *    anything else outside of the function.
         *
         * Some numbers:
         *
#{comment_details}
         *
         */
      headc
      headerc = headerc.strip.split("\n").map{|line| line = line.strip; line=' '+line if line[0..0]=='*'; line}
      headerc.join("\n")
    end

    def function
      return @function if @function
      raise ArgumentError,'No input items added yet' unless @inputs.size > 0
      parse_gperf_output

      #--- Lightly format hash value lookup table
      unless @parts[:hash_table].nil?
        ht = @parts[:hash_table].each_slice(10).map{|slc| slc.join(',')}
        ht = ht.each_slice(3).map{|b| ' '*8 + b.join(',  ')}.join(",\n")
      end
      @function = <<-final_function
#{f_sig} {
    #{@parts[:hash_table].nil? ? '' : "static const unsigned char asso_values[] = {
#{ht}};"}

#{f_out_table}
#{f_modified_calculation}
    if(__builtin_expect(!!(hval <= #{@parts[:max_hash_val]}), 1)) return #{f_returnval};
    return #{output_nomatch_val};
}
        final_function

      @function = @function.gsub(/\n+/,"\n") if @function.split("\n").size < 40
      @function = "\n" + f_header_comment + "\n" + @function
    end
  end
end
