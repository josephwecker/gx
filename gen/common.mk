CN = "\\033[0m"
CG = "\\033[32m"
CW = "\\033[30m\\033[1m"

define gcc
  $(call INFO, gcc, $(notdir $<), $@)
	@$(CC) $(1) $(patsubst %,-I%,$(VPATH)) -MP -MD -MF $B/$(notdir $@).d -o $@ $<
endef

define LINK
  $(call INFO, link, '*.o', $@)
	@$(CC) $(1) -I$B -o $@ $(filter %.o,$^)
endef

define INFO
	@printf "$(CG)  %-17s $(CN)%-18s $(CW)-->$(CN)  %s\n" $(notdir $1): $2 $3
endef

.FORCE : ;

.silent :
	@:

if_output_changed=$(if $(filter-out $(shell cat $1 2>/dev/null), $(shell $2)), $3)

