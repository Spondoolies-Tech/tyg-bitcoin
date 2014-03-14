SHAR_NAME			:= 'Spondoolies Software'
SHAR				:= spon.shar
SHAR_LATEST			:= spon-latest.shar
SCRIPT				:= ./sw-upgrade.sh

MAKESELF_FLAGS			:= --nocomp

ifeq ($(PRIVATE_KEY),)
$(warning Set PRIVATE_KEY: make PRIVATE_KEY=<priv key file>)
$(warning Example: make PRIVATE_KEY=/mnt/flash/spondoolies.pem)
$(error )
endif

all:				shar


shar:				data/uImage data/spondoolies.dtb
	makeself $(MAKESELF_FLAGS) data $(SHAR) "$(SHAR_NAME)" $(SCRIPT)
	../provisioning/sign-digest.sh --file=$(SHAR) --private=$(PRIVATE_KEY)

.PHONY:	latest
latest:
	makeself $(MAKESELF_FLAGS) latest $(SHAR_LATEST) "$(SHAR_NAME)" $(SCRIPT)
	../provisioning/sign-digest.sh --file=$(SHAR_LATEST) --private=$(PRIVATE_KEY)
	
