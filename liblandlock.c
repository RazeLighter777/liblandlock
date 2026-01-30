#include "liblandlock.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>

#if defined(__GLIBC__)
#include <linux/prctl.h>
#endif

#include <linux/netlink.h>

#ifndef NETLINK_SOCKET
#ifdef NETLINK_AUDIT
#define NETLINK_SOCKET NETLINK_AUDIT
#endif
#endif

#ifndef O_PATH
#define O_PATH 0
#endif

#ifndef landlock_create_ruleset
static inline int
landlock_create_ruleset(const struct landlock_ruleset_attr *const attr,
                        const size_t size, const __u32 flags)
{
    return syscall(__NR_landlock_create_ruleset, attr, size, flags);
}
#endif

#ifndef landlock_add_rule
static inline int landlock_add_rule(const int ruleset_fd,
                                    const enum landlock_rule_type rule_type,
                                    const void *const rule_attr,
                                    const __u32 flags)
{
    return syscall(__NR_landlock_add_rule, ruleset_fd, rule_type, rule_attr,
                   flags);
}
#endif

#ifndef landlock_restrict_self
static inline int landlock_restrict_self(const int ruleset_fd,
                                         const __u32 flags)
{
    return syscall(__NR_landlock_restrict_self, ruleset_fd, flags);
}
#endif

struct ll_ruleset
{
    int ruleset_fd;
    ll_abi_t abi;
    ll_abi_compat_mode_t compat_mode;
    __u64 handled_access_fs;
    __u64 handled_access_net;
    __u64 handled_access_scope;
};

static ll_error_t ll_error_with_errno(const int err, const ll_error_t code)
{
    (void)err;
    return code;
}

static ll_error_t ll_error_system(const int err)
{
    return ll_error_with_errno(err, LL_ERROR_SYSTEM);
}

static ll_error_t ll_error_from_create_ruleset_errno(const int err)
{
    switch (err)
    {
    case EOPNOTSUPP:
        return ll_error_with_errno(err, LL_ERROR_RULESET_CREATE_DISABLED);
    case EINVAL:
        return ll_error_with_errno(err, LL_ERROR_RULESET_CREATE_INVALID);
    case E2BIG:
        return ll_error_with_errno(err, LL_ERROR_RULESET_CREATE_SIZE_TOO_BIG);
    case EFAULT:
        return ll_error_with_errno(err, LL_ERROR_RULESET_CREATE_BAD_ADDRESS);
    case ENOMSG:
        return ll_error_with_errno(err, LL_ERROR_RULESET_CREATE_EMPTY_ACCESS);
    case ENOSYS:
        return ll_error_with_errno(err, LL_ERROR_UNSUPPORTED_SYSCALL);
    default:
        return ll_error_system(err);
    }
}

static ll_error_t ll_error_from_add_rule_errno(const int err)
{
    switch (err)
    {
    case EAFNOSUPPORT:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_TCP_UNSUPPORTED);
    case EOPNOTSUPP:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_DISABLED);
    case EINVAL:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS);
    case ENOMSG:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_EMPTY_ACCESS);
    case EBADF:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_BAD_FD);
    case EBADFD:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_BAD_FD_TYPE);
    case EPERM:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_NO_WRITE);
    case EFAULT:
        return ll_error_with_errno(err, LL_ERROR_ADD_RULE_BAD_ADDRESS);
    case ENOSYS:
        return ll_error_with_errno(err, LL_ERROR_UNSUPPORTED_SYSCALL);
    default:
        return ll_error_system(err);
    }
}

static ll_error_t ll_error_from_restrict_errno(const int err)
{
    switch (err)
    {
    case EOPNOTSUPP:
        return ll_error_with_errno(err, LL_ERROR_RESTRICT_DISABLED);
    case EINVAL:
        return ll_error_with_errno(err, LL_ERROR_RESTRICT_FLAGS_INVALID);
    case EBADF:
        return ll_error_with_errno(err, LL_ERROR_RESTRICT_BAD_FD);
    case EBADFD:
        return ll_error_with_errno(err, LL_ERROR_RESTRICT_BAD_FD_TYPE);
    case EPERM:
        return ll_error_with_errno(err, LL_ERROR_RESTRICT_NOT_PERMITTED);
    case E2BIG:
        return ll_error_with_errno(err, LL_ERROR_RESTRICT_LIMIT_REACHED);
    case ENOSYS:
        return ll_error_with_errno(err, LL_ERROR_UNSUPPORTED_SYSCALL);
    default:
        return ll_error_system(err);
    }
}

const char *ll_error_string(const ll_error_t error)
{
    switch (error)
    {
    case LL_ERROR_OK:
        return "Success.";
    case LL_ERROR_OK_PARTIAL_SANDBOX:
        return "Operation completed, but only partial sandboxing was applied.";
    case LL_ERROR_SYSTEM:
        return "System error.";
    case LL_ERROR_INVALID_ARGUMENT:
        return "Invalid argument.";
    case LL_ERROR_OUT_OF_MEMORY:
        return "Out of memory.";
    case LL_ERROR_UNSUPPORTED_SYSCALL:
        return "Required syscall is not available.";
    case LL_ERROR_RULESET_INCOMPATIBLE:
        return "Ruleset cannot be created due to compatibility checks.";
    case LL_ERROR_RULESET_CREATE_DISABLED:
        return "Landlock is supported by the kernel but disabled at boot time.";
    case LL_ERROR_RULESET_CREATE_INVALID:
        return "Unknown flags, or unknown access, or too small size.";
    case LL_ERROR_RULESET_CREATE_SIZE_TOO_BIG:
        return "size is too big.";
    case LL_ERROR_RULESET_CREATE_BAD_ADDRESS:
        return "attr was not a valid address.";
    case LL_ERROR_RULESET_CREATE_EMPTY_ACCESS:
        return "Empty accesses (i.e., attr did not specify any access rights to restrict).";
    case LL_ERROR_ADD_RULE_TCP_UNSUPPORTED:
        return "rule_type is LANDLOCK_RULE_NET_PORT, but TCP is not supported by the running kernel.";
    case LL_ERROR_ADD_RULE_DISABLED:
        return "Landlock is supported by the kernel but disabled at boot time.";
    case LL_ERROR_ADD_RULE_FLAGS_INVALID:
        return "flags is not 0.";
    case LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS:
        return "The rule accesses are inconsistent (i.e., rule_attr->allowed_access is not a subset of the ruleset handled accesses).";
    case LL_ERROR_ADD_RULE_ACCESS_NOT_APPLICABLE:
        return "In struct landlock_path_beneath_attr, the rule accesses are not applicable to the file (i.e., some access rights in rule_attr->allowed_access are only applicable to directories, but rule_attr->parent_fd does not refer to a directory).";
    case LL_ERROR_ADD_RULE_PORT_OUT_OF_RANGE:
        return "In struct landlock_net_port_attr, the port number is greater than 65535.";
    case LL_ERROR_ADD_RULE_EMPTY_ACCESS:
        return "Empty accesses (i.e., rule_attr->allowed_access is 0).";
    case LL_ERROR_ADD_RULE_BAD_FD:
        return "ruleset_fd is not a file descriptor for the current thread, or a member of rule_attr is not a file descriptor as expected.";
    case LL_ERROR_ADD_RULE_BAD_FD_TYPE:
        return "ruleset_fd is not a ruleset file descriptor, or a member of rule_attr is not the expected file descriptor type.";
    case LL_ERROR_ADD_RULE_NO_WRITE:
        return "ruleset_fd has no write access to the underlying ruleset.";
    case LL_ERROR_ADD_RULE_BAD_ADDRESS:
        return "rule_attr was not a valid address.";
    case LL_ERROR_RESTRICT_DISABLED:
        return "Landlock is supported by the kernel but disabled at boot time.";
    case LL_ERROR_RESTRICT_FLAGS_INVALID:
        return "flags is not 0.";
    case LL_ERROR_RESTRICT_BAD_FD:
        return "ruleset_fd is not a file descriptor for the current thread.";
    case LL_ERROR_RESTRICT_BAD_FD_TYPE:
        return "ruleset_fd is not a ruleset file descriptor.";
    case LL_ERROR_RESTRICT_NOT_PERMITTED:
        return "ruleset_fd has no read access to the underlying ruleset, or the calling thread is not running with no_new_privs, or it doesn't have the CAP_SYS_ADMIN in its user namespace.";
    case LL_ERROR_RESTRICT_LIMIT_REACHED:
        return "The maximum number of composed rulesets is reached for the calling thread.  This limit is currently 64.";
    case LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT:
        return "Sandbox partially applied, but was disallowed due to strict mode";
    default:
        return "Unknown error.";
    }
}

static ll_abi_t ll_resolve_abi(const ll_abi_t abi)
{
    if (abi == LL_ABI_LATEST)
    {
        const int ret = landlock_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
        if (ret < 0)
        {
            /* Fallback for kernels without Landlock support (ABI v1 baseline). */
            return 1;
        }
        return ret;
    }
    return abi;
}

static __u64 ll_supported_access_fs(const ll_abi_t abi)
{
    __u64 mask = 0;
    mask |= LANDLOCK_ACCESS_FS_EXECUTE;
    mask |= LANDLOCK_ACCESS_FS_WRITE_FILE;
    mask |= LANDLOCK_ACCESS_FS_READ_FILE;
    mask |= LANDLOCK_ACCESS_FS_READ_DIR;
    mask |= LANDLOCK_ACCESS_FS_REMOVE_DIR;
    mask |= LANDLOCK_ACCESS_FS_REMOVE_FILE;
    mask |= LANDLOCK_ACCESS_FS_MAKE_CHAR;
    mask |= LANDLOCK_ACCESS_FS_MAKE_DIR;
    mask |= LANDLOCK_ACCESS_FS_MAKE_REG;
    mask |= LANDLOCK_ACCESS_FS_MAKE_SOCK;
    mask |= LANDLOCK_ACCESS_FS_MAKE_FIFO;
    mask |= LANDLOCK_ACCESS_FS_MAKE_BLOCK;

    if (abi >= 2)
    {
        mask |= LANDLOCK_ACCESS_FS_REFER;
    }
    if (abi >= 3)
    {
        mask |= LANDLOCK_ACCESS_FS_TRUNCATE;
    }
    if (abi >= 5)
    {
        mask |= LANDLOCK_ACCESS_FS_IOCTL_DEV;
    }
    return mask;
}

static __u64 ll_supported_access_net(const ll_abi_t abi)
{
    if (abi < 4)
    {
        return 0;
    }
    return LANDLOCK_ACCESS_NET_BIND_TCP | LANDLOCK_ACCESS_NET_CONNECT_TCP;
}

static __u64 ll_supported_scopes(const ll_abi_t abi)
{
    if (abi < 6)
    {
        return 0;
    }
    return LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET | LANDLOCK_SCOPE_SIGNAL;
}

static __u32 ll_supported_restrict_self_flags(const ll_abi_t abi)
{
    if (abi < 7)
    {
        return 0;
    }
    return LANDLOCK_RESTRICT_SELF_LOG_SAME_EXEC_OFF |
           LANDLOCK_RESTRICT_SELF_LOG_NEW_EXEC_ON |
           LANDLOCK_RESTRICT_SELF_LOG_SUBDOMAINS_OFF;
}

static int ll_audit_supported(void)
{
#ifdef NETLINK_SOCKET
    const int audit_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_SOCKET);
    if (audit_fd < 0)
    {
        if (errno == EPROTONOSUPPORT)
        {
            return 0;
        }
        return 1;
    }
    close(audit_fd);
    return 1;
#else
    return 1;
#endif
}

ll_error_t ll_get_abi_version(ll_abi_t *const out_abi)
{
    if (!out_abi)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    const int ret = landlock_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (ret < 0)
    {
        return ll_error_from_create_ruleset_errno(errno);
    }

    *out_abi = ret;
    return LL_ERROR_OK;
}

ll_error_t ll_get_errata(int *const out_errata)
{
    if (!out_errata)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    const int ret = landlock_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_ERRATA);
    if (ret < 0)
    {
        return ll_error_from_create_ruleset_errno(errno);
    }

    *out_errata = ret;
    return LL_ERROR_OK;
}

ll_ruleset_attr_t ll_ruleset_attr_create(const ll_abi_t abi,
                                         const ll_abi_compat_mode_t compat_mode)
{
    ll_ruleset_attr_t ruleset_attr;
    memset(&ruleset_attr, 0, sizeof(ruleset_attr));
    ruleset_attr.abi = ll_resolve_abi(abi);
    ruleset_attr.compat_mode = compat_mode;
    return ruleset_attr;
}

ll_error_t ll_ruleset_attr_add_flags(ll_ruleset_attr_t *const ruleset_attr,
                                     const __u32 flags)
{
    if (!ruleset_attr)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    ruleset_attr->flags |= flags;
    return LL_ERROR_OK;
}

ll_error_t ll_ruleset_create(const ll_ruleset_attr_t ruleset_attr,
                             ll_ruleset_t **const out_ruleset)
{
    if (out_ruleset)
    {
        *out_ruleset = NULL;
    }

    if (!out_ruleset)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    const ll_abi_t policy_abi = ll_resolve_abi(ruleset_attr.abi);
    int kernel_abi = landlock_create_ruleset(NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (kernel_abi < 0)
    {
        return ll_error_from_create_ruleset_errno(errno);
    }

    if (ruleset_attr.compat_mode == LL_ABI_COMPAT_STRICT && kernel_abi < policy_abi)
    {
        return ll_error_with_errno(EOPNOTSUPP, LL_ERROR_RULESET_INCOMPATIBLE);
    }

    ll_abi_t effective_abi = policy_abi;
    if (ruleset_attr.compat_mode == LL_ABI_COMPAT_BEST_EFFORT && kernel_abi < policy_abi)
    {
        effective_abi = kernel_abi;
    }

    struct landlock_ruleset_attr attr = ruleset_attr.access;
    const __u64 fs_mask = ll_supported_access_fs(effective_abi);
    const __u64 net_mask = ll_supported_access_net(effective_abi);
    const __u64 scope_mask = ll_supported_scopes(effective_abi);

    const __u64 fs_before = attr.handled_access_fs;
    const __u64 net_before = attr.handled_access_net;
    const __u64 scope_before = attr.scoped;

    attr.handled_access_fs &= fs_mask;
    attr.handled_access_net &= net_mask;
    attr.scoped &= scope_mask;

    if (attr.handled_access_fs == 0 && attr.handled_access_net == 0 && attr.scoped == 0)
    {
        return ll_error_with_errno(ENOMSG, LL_ERROR_RULESET_CREATE_EMPTY_ACCESS);
    }

    const int create_flags = (int)(ruleset_attr.flags);
    const int ruleset_fd = landlock_create_ruleset(&attr, sizeof(attr), create_flags);
    if (ruleset_fd < 0)
    {
        return ll_error_from_create_ruleset_errno(errno);
    }

    ll_ruleset_t *ruleset = malloc(sizeof(*ruleset));
    if (!ruleset)
    {
        close(ruleset_fd);
        return ll_error_with_errno(ENOMEM, LL_ERROR_OUT_OF_MEMORY);
    }
    ruleset->ruleset_fd = ruleset_fd;
    ruleset->abi = effective_abi;
    ruleset->compat_mode = ruleset_attr.compat_mode;
    ruleset->handled_access_fs = attr.handled_access_fs;
    ruleset->handled_access_net = attr.handled_access_net;
    ruleset->handled_access_scope = attr.scoped;

    // Check for partial sandboxing in strict mode.
    if (ruleset_attr.compat_mode == LL_ABI_COMPAT_STRICT)
    {
        if (fs_before != ruleset->handled_access_fs ||
            net_before != ruleset->handled_access_net ||
            scope_before != ruleset->handled_access_scope)
        {
            ll_ruleset_close(ruleset);
            return ll_error_with_errno(EOPNOTSUPP, LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT);
        }
        *out_ruleset = ruleset;
        return LL_ERROR_OK;
    }
    else
    {
        *out_ruleset = ruleset;
        if (fs_before != ruleset->handled_access_fs ||
            net_before != ruleset->handled_access_net ||
            scope_before != ruleset->handled_access_scope)
        {
            return LL_ERROR_OK_PARTIAL_SANDBOX;
        }
        return LL_ERROR_OK;
    }
}

void ll_ruleset_close(ll_ruleset_t *const ruleset)
{
    if (!ruleset)
    {
        return;
    }

    if (ruleset->ruleset_fd >= 0)
    {
        close(ruleset->ruleset_fd);
    }
    free(ruleset);
}

ll_error_t ll_ruleset_add_path(const ll_ruleset_t *const ruleset,
                               const char *const path,
                               const __u64 access_masks,
                               const __u32 flags)
{
    if (!ruleset || ruleset->ruleset_fd < 0 || !path)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    const int dir_fd = open(path, O_PATH | O_CLOEXEC);
    if (dir_fd < 0)
    {
        return ll_error_system(errno);
    }

    const int ret = ll_ruleset_add_path_fd(ruleset, dir_fd, access_masks, flags);
    close(dir_fd);
    return ret;
}

ll_error_t ll_ruleset_add_path_fd(const ll_ruleset_t *const ruleset,
                                  const int dir_fd,
                                  const __u64 access_masks,
                                  const __u32 flags)
{
    if (!ruleset)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    if (ruleset->ruleset_fd < 0 || dir_fd < 0)
    {
        return ll_error_with_errno(EBADF, LL_ERROR_ADD_RULE_BAD_FD);
    }

    if (flags != 0)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_ADD_RULE_FLAGS_INVALID);
    }

    if (access_masks == 0)
    {
        return ll_error_with_errno(ENOMSG, LL_ERROR_ADD_RULE_EMPTY_ACCESS);
    }

    if ((access_masks & ~ruleset->handled_access_fs) != 0)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS);
    }

    const __u64 dir_only_bits = LANDLOCK_ACCESS_FS_READ_DIR |
                                LANDLOCK_ACCESS_FS_REMOVE_DIR |
                                LANDLOCK_ACCESS_FS_MAKE_DIR;
    if ((access_masks & dir_only_bits) != 0)
    {
        struct stat st;
        if (fstat(dir_fd, &st) != 0)
        {
            return ll_error_system(errno);
        }
        if (!S_ISDIR(st.st_mode))
        {
            return ll_error_with_errno(EINVAL, LL_ERROR_ADD_RULE_ACCESS_NOT_APPLICABLE);
        }
    }

    struct landlock_path_beneath_attr path_attr = {
        .allowed_access = access_masks,
        .parent_fd = dir_fd,
    };

    const int ret = landlock_add_rule(ruleset->ruleset_fd, LANDLOCK_RULE_PATH_BENEATH,
                                      &path_attr, flags);
    if (ret < 0)
    {
        return ll_error_from_add_rule_errno(errno);
    }
    return LL_ERROR_OK;
}

ll_error_t ll_ruleset_add_net_port(const ll_ruleset_t *const ruleset,
                                   const __u64 port,
                                   const __u64 access_masks,
                                   const __u32 flags)
{
    if (!ruleset)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    if (ruleset->ruleset_fd < 0)
    {
        return ll_error_with_errno(EBADF, LL_ERROR_ADD_RULE_BAD_FD);
    }

    if (flags != 0)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_ADD_RULE_FLAGS_INVALID);
    }

    if (access_masks == 0)
    {
        return ll_error_with_errno(ENOMSG, LL_ERROR_ADD_RULE_EMPTY_ACCESS);
    }

    if (port > 65535)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_ADD_RULE_PORT_OUT_OF_RANGE);
    }

    if (ruleset->handled_access_net == 0)
    {
        return ll_error_with_errno(EAFNOSUPPORT, LL_ERROR_ADD_RULE_TCP_UNSUPPORTED);
    }

    if ((access_masks & ~ruleset->handled_access_net) != 0)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_ADD_RULE_INCONSISTENT_ACCESS);
    }

    struct landlock_net_port_attr net_attr = {
        .allowed_access = access_masks,
        .port = port,
    };

    const int ret = landlock_add_rule(ruleset->ruleset_fd, LANDLOCK_RULE_NET_PORT, &net_attr, flags);
    if (ret < 0)
    {
        return ll_error_from_add_rule_errno(errno);
    }
    return LL_ERROR_OK;
}

ll_error_t ll_ruleset_enforce(const ll_ruleset_t *const ruleset,
                              const __u32 flags)
{
    if (!ruleset)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_INVALID_ARGUMENT);
    }

    if (ruleset->ruleset_fd < 0)
    {
        return ll_error_with_errno(EBADF, LL_ERROR_RESTRICT_BAD_FD);
    }

    const __u32 known_flags = LANDLOCK_RESTRICT_SELF_LOG_SAME_EXEC_OFF |
                              LANDLOCK_RESTRICT_SELF_LOG_NEW_EXEC_ON |
                              LANDLOCK_RESTRICT_SELF_LOG_SUBDOMAINS_OFF;
    if ((flags & ~known_flags) != 0)
    {
        return ll_error_with_errno(EINVAL, LL_ERROR_RESTRICT_FLAGS_INVALID);
    }

    const __u32 supported = ll_supported_restrict_self_flags(ruleset->abi);
    __u32 masked_flags = flags & supported;
    if (ruleset->compat_mode == LL_ABI_COMPAT_STRICT && masked_flags != flags)
    {
        return ll_error_with_errno(EOPNOTSUPP, LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT);
    }

    if (masked_flags != 0 && !ll_audit_supported())
    {
        if (ruleset->compat_mode == LL_ABI_COMPAT_STRICT)
        {
            return ll_error_with_errno(EOPNOTSUPP, LL_ERROR_RESTRICT_PARTIAL_SANDBOX_STRICT);
        }
        masked_flags = 0;
    }

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0))
    {
        return ll_error_system(errno);
    }

    const int ret = landlock_restrict_self(ruleset->ruleset_fd, masked_flags);
    if (ret < 0)
    {
        return ll_error_from_restrict_errno(errno);
    }
    return LL_ERROR_OK;
}