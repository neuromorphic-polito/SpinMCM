# If SPINN_DIRS is not defined, this is an error!
ifndef SPINN_DIRS
    $(error SPINN_DIRS is not set. Please define SPINN_DIRS (possibly by running "source setup" in the spinnaker tools folder))
endif

SPINN_TWO_BUILD = build
include $(SPINN_DIRS)/make/Makefile.common

#SPINN_COMMON_DEBUG := PRODUCTION_CODE

# Include our own include directory
CFLAGS += -I include $(OTIME) # -D$(SPINN_COMMON_DEBUG)

# Objects
OBJS = spin2_ht.o spin2_mc.o spin2_sdp.o
BUILD_OBJS = $(OBJS:%.o=$(SPINN_TWO_BUILD)/%.o)

# Headers
HEADERS = spin2_api.h
INSTALL_HEADERS = $(HEADERS:%.h=$(SPINN_INC_DIR)/%.h)

# Build rules (default)
$(SPINN_TWO_BUILD)/libspinn_two.a: $(BUILD_OBJS) 
	$(RM) $@
	$(AR) $@ $(BUILD_OBJS)

$(SPINN_TWO_BUILD)/%.o: src/%.c $(SPINN_TWO_BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(SPINN_TWO_BUILD):
	$(MKDIR) $@

# Installing rules
install: $(SPINN_LIB_DIR)/libspinn_two.a $(INSTALL_HEADERS)

$(SPINN_LIB_DIR)/libspinn_two.a: $(SPINN_TWO_BUILD)/libspinn_two.a
	$(CP) $< $@

$(SPINN_INC_DIR)/%.h: include/%.h
	$(CP) $< $@

clean:
	$(RM) $(SPINN_TWO_BUILD)/libspinn_two.a $(BUILD_OBJS)
