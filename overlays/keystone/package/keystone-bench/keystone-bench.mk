################################################################################
#
# Keystone bench
#
################################################################################

ifeq ($(KEYSTONE_BENCH),)
$(error KEYSTONE_BENCH directory not defined)
else
include $(KEYSTONE)/mkutils/pkg-keystone.mk
endif

KEYSTONE_BENCH_DEPENDENCIES += host-keystone-sdk keystone-runtime
ifeq ($(KEYSTONE_PLATFORM),mpfs)
KEYSTONE_BENCH_DEPENDENCIES += hss
KEYSTONE_BENCH_CONF_OPTS += -Dfw_bin=$(BINARIES_DIR)/hss-l2scratch.bin
else
KEYSTONE_BENCH_DEPENDENCIES += opensbi
endif

KEYSTONE_BENCH_CONF_OPTS += -DKEYSTONE_SDK_DIR=$(HOST_DIR)/usr/share/keystone/sdk \
                                -DKEYSTONE_EYRIE_RUNTIME=$(KEYSTONE_RUNTIME_BUILDDIR) \
                                -DKEYSTONE_BITS=${KEYSTONE_BITS}
ifeq ($(KEYSTONE_PLATFORM),cva6)
KEYSTONE_BENCH_CONF_OPTS += -Dfw_bin=$(BINARIES_DIR)/fw_payload.bin
endif

KEYSTONE_BENCH_MAKE_ENV += KEYSTONE_SDK_DIR=$(HOST_DIR)/usr/share/keystone/sdk
KEYSTONE_BENCH_MAKE_OPTS += bench

# Install only .ke files
define KEYSTONE_BENCH_INSTALL_TARGET_CMDS
	find $(@D) -name '*.ke' | \
                xargs -i{} $(INSTALL) -D -m 755 -t $(TARGET_DIR)/usr/share/keystone/bench/ {}
endef

$(eval $(keystone-package))
$(eval $(cmake-package))


