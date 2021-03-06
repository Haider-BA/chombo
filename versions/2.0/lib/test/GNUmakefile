# -*- Mode: Makefile; Modified: "Fri 18 Feb 2005 15:11:13 by dbs"; -*- 

### This makefile controls building and running Chombo test programs.
### It is nearly identical to `$CHOMBO_HOME/src/GNUmakefile'.

### The way this makefile works is the target specified by the user
### on the command line is treated as the `action' to perform on all
### the `test' subdirectories.  In the rules, the subdirectories are
### treated like targets and the action is specified by a variable.
### The makefile builds each directory target by invoking make
### recursively on the directory with the action as the recursive target.
### This assumes that the makefiles in the subdirectories implement all
### the actions in the $(_all_actions) variable.

export makefiles+=lib_test

## This variable specifies where we are.  Lower-level make files will need an
## absolute path so we use 'pwd' rather than "."
export CHOMBO_HOME := $(shell pwd)/..

##
## These variables define all the subdirectories that can be built.
## These library names are used by the makefiles in the application directories.
##[NOTE: if you change the names, change the dependencies below.]

lib_ch_targets := BoxTools AMRTools AMRElliptic EllipticDesign AMRTimeDependent

lib_targets := $(lib_ch_targets)

# some src subdirs don't have tests; ChomboFortran and DotProduct have tests but no src subdir
#[NOTE: this has to be "=", not ":=" for it to work correctly]
test_targets = $(filter-out InfiniteDomain,$(lib_targets)) ChomboFortran

##
## These are the actions (targets for the recursive make) supported by this
## makefile.  It assumes the makefiles in the subdirectories support them too.
##   all        builds all the executables, including the libraries they depend on
##   test       builds all the executables, but not the libraries
##   run        runs all the executables
##   clean      deletes files for this configuration
##   realclean  deletes files for all configurations
##   vars       prints values of various make variables
##
_all_actions = all test run clean realclean

.PHONY: $(_all_actions) $(test_targets) usage vars

##
## Default target
##
usage:
	@echo "This makefile supports targets:"
	@echo "     $(_all_actions) $(test_targets) usage vars"

##
## For each action, set the action variable to tell the recursive
## makefile what to do.  There should be a separate dependency for
## every value in $(_all_actions).
##[NOTE: the `NODEPENDS=TRUE' variable disables automatic
##       dependency analysis in the recursive makefile, which is only
##       needed for actions that compile or link code.]
##
_action = all
all       : _action = all
test      : _action = test
run       : _action = run       NODEPENDS=TRUE
clean     : _action = clean     NODEPENDS=TRUE
realclean : _action = realclean NODEPENDS=TRUE

# this dependency ensures the action is done for every directory.
$(_all_actions) : $(test_targets)

# this rule performs the action
$(test_targets):
	$(MAKE) --no-print-directory --directory $@ $(_action)

# this rule prints out various 'make' variables to the screen for debugging
vars:
	@$(MAKE) --no-print-directory -f $(CHOMBO_HOME)/mk/Make.printVariables $@
