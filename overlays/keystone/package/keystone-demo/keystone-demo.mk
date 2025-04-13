################################################################################
#
# Keystone demo
#
################################################################################

ifeq ($(KEYSTONE_DEMO),)
$(error KEYSTONE_DEMO directory not defined)
else
include $(KEYSTONE)/mkutils/pkg-keystone.mk
endif

KEYSTONE_DEMO_DEPENDENCIES += host-keystone-sdk keystone-runtime
ifeq ($(KEYSTONE_PLATFORM),mpfs)
KEYSTONE_DEMO_DEPENDENCIES += hss
KEYSTONE_DEMO_CONF_OPTS += -Dfw_bin=$(BINARIES_DIR)/hss-l2scratch.bin
else
KEYSTONE_DEMO_DEPENDENCIES += opensbi
endif

KEYSTONE_DEMO_CONF_OPTS += -DKEYSTONE_SDK_DIR=$(HOST_DIR)/usr/share/keystone/sdk \
                                -DKEYSTONE_EYRIE_RUNTIME=$(KEYSTONE_RUNTIME_BUILDDIR) \
                                -DKEYSTONE_BITS=${KEYSTONE_BITS}
ifeq ($(KEYSTONE_PLATFORM),cva6)
KEYSTONE_DEMO_CONF_OPTS += -Dfw_bin=$(BINARIES_DIR)/fw_payload.bin
endif

KEYSTONE_DEMO_MAKE_ENV += KEYSTONE_SDK_DIR=$(HOST_DIR)/usr/share/keystone/sdk
# KEYSTONE_DEMO_MAKE_OPTS += demo

# Install only .ke files, riscv files, eyrie-rt, loader.bin
define KEYSTONE_DEMO_INSTALL_TARGET_CMDS
	find $(@D) -name '*.ke' | \
                xargs -i{} $(INSTALL) -D -m 755 -t $(TARGET_DIR)/usr/share/keystone/demo/ {} ; \
    find $(@D) -name '*riscv' | \
                xargs -i{} $(INSTALL) -D -m 755 -t $(TARGET_DIR)/usr/share/keystone/demo/ {} ; \
    find $(@D) -name 'eyrie-rt' | \
                xargs -i{} $(INSTALL) -D -m 755 -t $(TARGET_DIR)/usr/share/keystone/demo/ {} ; \
    find $(@D) -name 'loader.bin' | \
                xargs -i{} $(INSTALL) -D -m 755 -t $(TARGET_DIR)/usr/share/keystone/demo/ {} ; \
    find $(@D) -name '.options_log' | \
                xargs -i{} $(INSTALL) -D -m 755 -t $(TARGET_DIR)/usr/share/keystone/demo/ {}
                

endef

$(eval $(keystone-package))
$(eval $(cmake-package))


