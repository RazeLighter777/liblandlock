#pragma once
#include "linux/landlock.h"
#include <stddef.h>

/**
 * @brief Landlock ABI version selector used by this library.
 */
typedef int ll_abi_t;
/**
 * @brief Select the latest ABI supported by the running kernel.
 */
#define LL_ABI_LATEST 0

/**
 * @brief Compatibility policy for ABI mismatch handling.
 */
typedef enum
{
    /**
     * @brief Require exact ABI support; reject unsupported features.
     */
    LL_ABI_COMPAT_STRICT = -1,
    /**
     * @brief Allow downgrades; silently mask unsupported features.
     */
    LL_ABI_COMPAT_BEST_EFFORT = 0,
} ll_abi_compat_mode_t;

/**
 * @brief Landlock error codes (negative errno values).
 */
typedef enum
{
    /**
     * @brief Operation completed successfully.
     */
    LL_ERROR_OK = 0,
    /**
     * @brief Operation completed, but only partial sandboxing was applied.
     */
    LL_ERROR_OK_PARTIAL_SANDBOX = 1,
    /**
     * @brief System error not covered by Landlock semantics.
     */
    LL_ERROR_SYSTEM = -1,
    /**
     * @brief Invalid argument provided to the library.
     */
    LL_ERROR_INVALID_ARGUMENT = -2,
    /**
     * @brief Out of memory.
     */
    LL_ERROR_OUT_OF_MEMORY = -3,
    /**
     * @brief Required syscall is not available.
     */
    LL_ERROR_UNSUPPORTED_SYSCALL = -5,
    /**
     * @brief Ruleset cannot be created due to compatibility checks.
     */
    LL_ERROR_RULESET_INCOMPATIBLE = -6,

    /**
     * @brief Landlock is supported by the kernel but disabled at boot time.
     */
    LL_ERROR_RULESET_CREATE_DISABLED = -100,
    /**
     * @brief Unknown flags, or unknown access, or too small size.
     */
    LL_ERROR_RULESET_CREATE_INVALID = -101,
    /**
     * @brief size is too big.
     */
    LL_ERROR_RULESET_CREATE_SIZE_TOO_BIG = -102,
    /**
     * @brief attr was not a valid address.
     */
    LL_ERROR_RULESET_CREATE_BAD_ADDRESS = -103,
    /**
     * @brief Empty accesses (i.e., attr did not specify any access rights to restrict).
     */
    LL_ERROR_RULESET_CREATE_EMPTY_ACCESS = -104,

    /**
     * @brief rule_type is LANDLOCK_RULE_NET_PORT, but TCP is not supported by the running kernel.
     */
    LL_ERROR_ADD_RULE_TCP_UNSUPPORTED = -120,
    /**
     * @brief Landlock is supported by the kernel but disabled at boot time.
     */
    LL_ERROR_ADD_RULE_DISABLED = -121,
    /**
     * @brief flags is not 0.
     */
    LL_ERROR_ADD_RULE_FLAGS_INVALID = -122,
    /**
     * @brief The rule accesses are inconsistent (i.e., rule_attr->allowed_access is not a subset of the ruleset handled accesses).
     */
    LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS = -123,
    /**
     * @brief In struct landlock_path_beneath_attr, the rule accesses are not applicable to the file (i.e., some access rights in rule_attr->allowed_access are only applicable to directories, but rule_attr->parent_fd does not refer to a directory).
     */
    LL_ERROR_ADD_RULE_ACCESS_NOT_APPLICABLE = -124,
    /**
     * @brief In struct landlock_net_port_attr, the port number is greater than 65535.
     */
    LL_ERROR_ADD_RULE_PORT_OUT_OF_RANGE = -125,
    /**
     * @brief Empty accesses (i.e., rule_attr->allowed_access is 0).
     */
    LL_ERROR_ADD_RULE_EMPTY_ACCESS = -126,
    /**
     * @brief ruleset_fd is not a file descriptor for the current thread, or a member of rule_attr is not a file descriptor as expected.
     */
    LL_ERROR_ADD_RULE_BAD_FD = -127,
    /**
     * @brief ruleset_fd is not a ruleset file descriptor, or a member of rule_attr is not the expected file descriptor type.
     */
    LL_ERROR_ADD_RULE_BAD_FD_TYPE = -128,
    /**
     * @brief ruleset_fd has no write access to the underlying ruleset.
     */
    LL_ERROR_ADD_RULE_NO_WRITE = -129,
    /**
     * @brief rule_attr was not a valid address.
     */
    LL_ERROR_ADD_RULE_BAD_ADDRESS = -130,

    /**
     * @brief Landlock is supported by the kernel but disabled at boot time.
     */
    LL_ERROR_RESTRICT_DISABLED = -140,
    /**
     * @brief flags is not supported.
     */
    LL_ERROR_RESTRICT_FLAGS_INVALID = -141,
    /**
     * @brief ruleset_fd is not a file descriptor for the current thread.
     */
    LL_ERROR_RESTRICT_BAD_FD = -142,
    /**
     * @brief ruleset_fd is not a ruleset file descriptor.
     */
    LL_ERROR_RESTRICT_BAD_FD_TYPE = -143,
    /**
     * @brief ruleset_fd has no read access to the underlying ruleset, or the calling thread is not running with no_new_privs, or it doesn't have the CAP_SYS_ADMIN in its user namespace.
     */
    LL_ERROR_RESTRICT_NOT_PERMITTED = -144,
    /**
     * @brief The maximum number of composed rulesets is reached for the calling thread.  This limit is currently 64.
     */
    LL_ERROR_RESTRICT_LIMIT_REACHED = -145,
    /**
     * @brief Sandbox partially applied, but was disallowed due to strict mode
     */
    LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT = -146,
} ll_error_t;

#define LL_ERRORED(err) ((err) < LL_ERROR_OK)

/**
 * @brief Map a Landlock error code to a human-readable string.
 *
 * @param error Error code (negative errno value).
 * @return String description for known errors.
 */
const char *ll_error_string(ll_error_t error);

/**
 * @brief Opaque ruleset handle.
 */
typedef struct ll_ruleset ll_ruleset_t;

/**
 * @brief Access class selector for ruleset attributes.
 */
typedef enum
{
    /**
     * @brief Filesystem access rights.
     */
    LL_RULESET_ACCESS_CLASS_FS = 0,
    /**
     * @brief Network access rights.
     */
    LL_RULESET_ACCESS_CLASS_NET = 1,
    /**
     * @brief Sandbox scope rights.
     */
    LL_RULESET_ACCESS_CLASS_SCOPE = 2,
} ll_ruleset_access_class_t;

/**
 * @brief Convenience filesystem read access group.
 */
#define LL_ACCESS_GROUP_FS_READ (LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR)

/**
 * @brief Convenience filesystem write access group.
 */
#define LL_ACCESS_GROUP_FS_WRITE (LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_REMOVE_DIR | \
                                  LANDLOCK_ACCESS_FS_REMOVE_FILE | LANDLOCK_ACCESS_FS_MAKE_CHAR | \
                                  LANDLOCK_ACCESS_FS_MAKE_DIR | LANDLOCK_ACCESS_FS_MAKE_REG |     \
                                  LANDLOCK_ACCESS_FS_MAKE_SOCK | LANDLOCK_ACCESS_FS_MAKE_FIFO |   \
                                  LANDLOCK_ACCESS_FS_MAKE_BLOCK | LANDLOCK_ACCESS_FS_MAKE_SYM |   \
                                  LANDLOCK_ACCESS_FS_REFER)

/**
 * @brief Convenience filesystem execute access group (includes read).
 */
#define LL_ACCESS_GROUP_FS_EXECUTE (LANDLOCK_ACCESS_FS_EXECUTE | LL_ACCESS_GROUP_FS_READ)

/**
 * @brief Convenience filesystem all access group.
 */
#define LL_ACCESS_GROUP_FS_ALL (LL_ACCESS_GROUP_FS_READ | LL_ACCESS_GROUP_FS_WRITE | LL_ACCESS_GROUP_FS_EXECUTE)

/**
 * @brief Convenience network connect access group.
 */
#define LL_ACCESS_GROUP_NET_CONNECT (LANDLOCK_ACCESS_NET_CONNECT_TCP)

/**
 * @brief Convenience network bind access group.
 */
#define LL_ACCESS_GROUP_NET_BIND (LANDLOCK_ACCESS_NET_BIND_TCP)

/**
 * @brief Convenience network all access group.
 */
#define LL_ACCESS_GROUP_NET_ALL (LL_ACCESS_GROUP_NET_CONNECT | LL_ACCESS_GROUP_NET_BIND)

/**
 * @brief Ruleset attributes used before creating a ruleset.
 */
typedef struct
{
    ll_abi_t abi;
    ll_abi_compat_mode_t compat_mode;
    struct landlock_ruleset_attr attr;
    __u32 flags;
} ll_ruleset_attr_t;

/**
 * @brief Query the Landlock ABI version supported by the running kernel.
 *
 * @param out_abi Output ABI version on success.
 * @return LL_ERROR_OK on success, negative @ref ll_error_t on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL output pointer).
 * @retval LL_ERROR_RULESET_CREATE_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_RULESET_CREATE_INVALID Invalid flags/access/size passed to ruleset creation.
 * @retval LL_ERROR_RULESET_CREATE_SIZE_TOO_BIG Ruleset attribute size too big.
 * @retval LL_ERROR_RULESET_CREATE_BAD_ADDRESS Invalid attribute address.
 * @retval LL_ERROR_RULESET_CREATE_EMPTY_ACCESS Empty handled access set.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
ll_error_t ll_get_abi_version(ll_abi_t *const out_abi);

/**
 * @brief Query Landlock errata bitmask supported by the running kernel.
 *
 * @param out_errata Output errata bitmask on success.
 * @return LL_ERROR_OK on success, negative @ref ll_error_t on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL output pointer).
 * @retval LL_ERROR_RULESET_CREATE_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_RULESET_CREATE_INVALID Invalid flags/access/size passed to ruleset creation.
 * @retval LL_ERROR_RULESET_CREATE_SIZE_TOO_BIG Ruleset attribute size too big.
 * @retval LL_ERROR_RULESET_CREATE_BAD_ADDRESS Invalid attribute address.
 * @retval LL_ERROR_RULESET_CREATE_EMPTY_ACCESS Empty handled access set.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
ll_error_t ll_get_errata(int *const out_errata);

/**
 * @brief Initialize a ruleset attribute container.
 *
 * @param abi Requested ABI version or LL_ABI_LATEST.
 * @param compat_mode Compatibility policy when the kernel ABI is lower than requested.
 * @return Initialized attribute container with zeroed access masks.
 */
__attribute__((warn_unused_result)) ll_ruleset_attr_t ll_ruleset_attr_make(ll_abi_t abi,
                                                                           ll_abi_compat_mode_t compat_mode);

static inline ll_ruleset_attr_t ll_ruleset_attr_defaults(void)
{
    return ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);
}
/**
 * @brief Allow access in a given domain for a ruleset attribute container.
 *
 * @param ruleset_attr Attribute container to modify.
 * @param access_class Access class of the mask.
 * @param access_requested Access mask to allow.
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL container or unknown access class).
 * @retval LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT Requested access not supported in strict mode.
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_attr_handle(ll_ruleset_attr_t *const ruleset_attr,
                                                                      ll_ruleset_access_class_t access_class,
                                                                      const __u64 access_requested);

static inline ll_error_t ll_ruleset_attr_handle_fs(ll_ruleset_attr_t *const ruleset_attr,
                                                   const __u64 access_requested)
{
    return ll_ruleset_attr_handle(ruleset_attr, LL_RULESET_ACCESS_CLASS_FS, access_requested);
}

static inline ll_error_t ll_ruleset_attr_handle_net(ll_ruleset_attr_t *const ruleset_attr,
                                                    const __u64 access_requested)
{
    return ll_ruleset_attr_handle(ruleset_attr, LL_RULESET_ACCESS_CLASS_NET, access_requested);
}

static inline ll_error_t ll_ruleset_attr_handle_scope(ll_ruleset_attr_t *const ruleset_attr,
                                                      const __u64 access_requested)
{
    return ll_ruleset_attr_handle(ruleset_attr, LL_RULESET_ACCESS_CLASS_SCOPE, access_requested);
}

/**
 * @brief Add flags to ruleset attributes.
 * @param ruleset_attr Attribute container to modify.
 * @param flags Flags to add.
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL container).
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_attr_add_flags(ll_ruleset_attr_t *const ruleset_attr,
                                                                         const __u32 flags);

/**
 * @brief Create a ruleset from prepared attributes.
 *
 * @param ruleset_attr Initialized attributes.
 * @param result Optional sandboxing result summary.
 * @param out_ruleset Output handle on success.
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL attributes or output handle).
 * @retval LL_ERROR_RULESET_INCOMPATIBLE Requested ABI is not supported in strict mode.
 * @retval LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT Requested access not supported in strict mode.
 * @retval LL_ERROR_OUT_OF_MEMORY Allocation failed.
 * @retval LL_ERROR_RULESET_CREATE_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_RULESET_CREATE_INVALID Invalid flags/access/size passed to ruleset creation.
 * @retval LL_ERROR_RULESET_CREATE_SIZE_TOO_BIG Ruleset attribute size too big.
 * @retval LL_ERROR_RULESET_CREATE_BAD_ADDRESS Invalid attribute address.
 * @retval LL_ERROR_RULESET_CREATE_EMPTY_ACCESS Empty handled access set.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_create(const ll_ruleset_attr_t *const ruleset_attr,
                                                                 ll_ruleset_t **const out_ruleset);

/**
 * @brief Close and free a ruleset handle.
 *
 * @param ruleset Ruleset handle to close (may be NULL).
 */
void ll_ruleset_close(ll_ruleset_t *const ruleset);

/**
 * @brief Add a path-beneath rule to a ruleset.
 *
 * @param ruleset Ruleset handle.
 * @param path Path to the directory to grant access to.
 * @param access_masks Access mask for the path.
 * @param flags Flags passed to landlock_add_rule().
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL ruleset or path).
 * @retval LL_ERROR_ADD_RULE_FLAGS_INVALID flags is not 0.
 * @retval LL_ERROR_ADD_RULE_EMPTY_ACCESS Empty accesses.
 * @retval LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS Access not covered by handled accesses.
 * @retval LL_ERROR_ADD_RULE_ACCESS_NOT_APPLICABLE Access requires a directory, but FD is not a directory.
 * @retval LL_ERROR_ADD_RULE_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_ADD_RULE_BAD_FD Ruleset or attribute FD is invalid.
 * @retval LL_ERROR_ADD_RULE_BAD_FD_TYPE Ruleset FD is not a ruleset FD.
 * @retval LL_ERROR_ADD_RULE_NO_WRITE Ruleset has no write access.
 * @retval LL_ERROR_ADD_RULE_BAD_ADDRESS Invalid rule attribute address.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_add_path(const ll_ruleset_t *const ruleset,
                                                                   const char *const path,
                                                                   const __u64 access_masks,
                                                                   const __u32 flags);

/**
 * @brief Add a path-beneath rule to a ruleset using an existing FD.
 *
 * @param ruleset Ruleset handle.
 * @param dir_fd Directory file descriptor (e.g., opened with O_PATH).
 * @param access_masks Access mask for the path.
 * @param flags Flags passed to landlock_add_rule().
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL ruleset).
 * @retval LL_ERROR_ADD_RULE_BAD_FD Ruleset or directory FD is invalid.
 * @retval LL_ERROR_ADD_RULE_FLAGS_INVALID flags is not 0.
 * @retval LL_ERROR_ADD_RULE_EMPTY_ACCESS Empty accesses.
 * @retval LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS Access not covered by handled accesses.
 * @retval LL_ERROR_ADD_RULE_ACCESS_NOT_APPLICABLE Access requires a directory, but FD is not a directory.
 * @retval LL_ERROR_ADD_RULE_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_ADD_RULE_BAD_FD_TYPE Ruleset FD is not a ruleset FD.
 * @retval LL_ERROR_ADD_RULE_NO_WRITE Ruleset has no write access.
 * @retval LL_ERROR_ADD_RULE_BAD_ADDRESS Invalid rule attribute address.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_add_path_fd(const ll_ruleset_t *const ruleset,
                                                                      const int dir_fd,
                                                                      const __u64 access_masks,
                                                                      const __u32 flags);

/**
 * @brief Add a network port rule to a ruleset.
 *
 * @param ruleset Ruleset handle.
 * @param port TCP port to grant access to.
 * @param access_masks Access mask for the port.
 * @param flags Flags passed to landlock_add_rule().
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL ruleset).
 * @retval LL_ERROR_ADD_RULE_BAD_FD Ruleset FD is invalid.
 * @retval LL_ERROR_ADD_RULE_FLAGS_INVALID flags is not 0.
 * @retval LL_ERROR_ADD_RULE_EMPTY_ACCESS Empty accesses.
 * @retval LL_ERROR_ADD_RULE_PORT_OUT_OF_RANGE Port is greater than 65535.
 * @retval LL_ERROR_ADD_RULE_TCP_UNSUPPORTED TCP is not supported by the running kernel.
 * @retval LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS Access not covered by handled accesses.
 * @retval LL_ERROR_ADD_RULE_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_ADD_RULE_BAD_FD_TYPE Ruleset FD is not a ruleset FD.
 * @retval LL_ERROR_ADD_RULE_NO_WRITE Ruleset has no write access.
 * @retval LL_ERROR_ADD_RULE_BAD_ADDRESS Invalid rule attribute address.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_add_net_port(const ll_ruleset_t *const ruleset,
                                                                       const __u64 port,
                                                                       const __u64 access_masks,
                                                                       const __u32 flags);

/**
 * @brief Enforce the ruleset on the current process.
 *
 * @param ruleset Ruleset handle.
 * @param flags Flags passed to landlock_restrict_self().
 * @return LL_ERROR_OK on success, negative error code on failure.
 *
 * @retval LL_ERROR_OK Success.
 * @retval LL_ERROR_INVALID_ARGUMENT Invalid argument (e.g., NULL ruleset).
 * @retval LL_ERROR_RESTRICT_BAD_FD Ruleset FD is invalid.
 * @retval LL_ERROR_RESTRICT_FLAGS_INVALID Unknown flags set.
 * @retval LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT Requested flags not supported in strict mode.
 * @retval LL_ERROR_RESTRICT_DISABLED Landlock is supported but disabled at boot time.
 * @retval LL_ERROR_RESTRICT_BAD_FD_TYPE Ruleset FD is not a ruleset FD.
 * @retval LL_ERROR_RESTRICT_NOT_PERMITTED Insufficient permissions or no_new_privs not set.
 * @retval LL_ERROR_RESTRICT_LIMIT_REACHED Maximum composed rulesets reached.
 * @retval LL_ERROR_UNSUPPORTED_SYSCALL Required syscall not available.
 * @retval LL_ERROR_SYSTEM Other system error.
 */
__attribute__((warn_unused_result)) ll_error_t ll_ruleset_enforce(const ll_ruleset_t *const ruleset,
                                                                  const __u32 flags);