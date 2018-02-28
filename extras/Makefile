#
# Sample Makefile for use with nwnsc
#
# Notes for Windows users
#    make is not installed by default on Windows, you'll need to download it.
#    The easiest option is:
#        http://www.equation.com/servlet/equation.cmd?fa=make
#    as that comes without any dependencies. Other options are Cygwin and MinGW.

#
# Examples of commands to run:
#   make batch       - Compile everything in a single batch call
#   make -j8         - Compile everything, one call per file, on 8 threads
#   make clean       - Remove all .ncs and .d files
#   make VERBOSE=1   - Compile everything, one call per file, extra verbose output
#   make 

#
# Set up your paths. These values can also be set as environment variables, and
# they will overwrite what is specified here.
#
# You can also override them from the command line, like:
#     make NWNDIR=path/to/nwn NWNHOMEDIR=path/to/home

# Path to nwnsc compiler
NWNSC ?= ../nwnsc/nwnsc
# Path to the NWN:EE home directory
NWNHOMEDIR ?= "~/.local/share/Neverwinter Nights"
# Install directory for NWN:EE
NWNDIR ?= "~/Beamdog Library/00829"
# Path to folder with scripts to be compiled
SCRIPTS ?= .
# Where the compiled NCS files are to be stored
OUTPUTDIR ?= $(SCRIPTS)
# Any additional dirs to look into when including files
INCLUDE_DIRS ?= 
# Whether to print verbose info while compiling
VERBOSE ?=

# NSS is a list of all scripts to compile
NSS := $(wildcard $(SCRIPTS)/*.nss)
# NCS is a list of all final compiled scripts
NCS := $(addprefix $(OUTPUTDIR)/,$(notdir $(NSS:.nss=.ncs)))

# Additional flags to pass to the compiler
NSCFLAGS ?= -ole

# Add scripts dir to includes includes if not current dir
ifneq ("$(SCRIPTS)",".")
  INCLUDE_DIRS += $(SCRIPTS)
endif

ifneq ("$(INCLUDE_DIRS)","")
  INCLUDE_DIRS := $(addprefix -i ,$(INCLUDE_DIRS))
endif

ifeq ("$(VERBOSE)","")
  Q = @
  NSCFLAGS += -q
else
  Q =
endif

build:: $(NCS)

$(OUTPUTDIR)/%.ncs: $(SCRIPTS)/%.nss
	$(Q)echo [nwnsc] $@
	$(Q)$(NWNSC) $(NSCFLAGS) -n $(NWNDIR) -i "$(NWNDIR)/ovr" $(INCLUDE_DIRS) -i $(SCRIPTS) -M -r $@ $<

batch:
	$(Q)echo [nwnsc] $(NCS)
	$(Q)$(NWNSC) $(NSCFLAGS) -n $(NWNDIR) -i "$(NWNDIR)/ovr" $(INCLUDE_DIRS) -i $(SCRIPTS) -M $(NSS)

.PHONY: clean batch
clean:
	-$(Q)rm -f $(NCS) $(NCS:.ncs=.d)

-include $(OUTPUTDIR)/*.d
