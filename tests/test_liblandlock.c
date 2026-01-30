#ifdef LL_TEST_HEADER_ONLY
#define LIBLANDLOCK_IMPLEMENTATION
#include "../dist/liblandlock.h"
#else
#include "../liblandlock.h"
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef O_PATH
#define O_PATH 0
#endif

static int tests_failed = 0;

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    tests_failed++;
}

static void test_create_attr_defaults(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);
    if (attr.attr.handled_access_fs != 0)
    {
        fail("handled_access_fs should default to 0");
    }
    if (attr.attr.handled_access_net != 0)
    {
        fail("handled_access_net should default to 0");
    }
    if (attr.attr.scoped != 0)
    {
        fail("scoped should default to 0");
    }
    if (attr.compat_mode != LL_ABI_COMPAT_BEST_EFFORT)
    {
        fail("compat_mode should match the requested mode");
    }
}

static void test_handle_access_fs_strict(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(1, LL_ABI_COMPAT_STRICT);
    ll_error_t ret = ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS, LANDLOCK_ACCESS_FS_REFER);
    if (ret != LL_ERROR_UNSUPPORTED_FEATURE)
    {
        fail("strict mode should reject unsupported FS access");
    }
}

static void test_handle_access_fs_best_effort(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(1, LL_ABI_COMPAT_BEST_EFFORT);
    ll_error_t ret = ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS, LANDLOCK_ACCESS_FS_REFER);
    if (ret != 0)
    {
        fail("best-effort should allow unsupported FS access request");
    }
    if ((attr.attr.handled_access_fs & LANDLOCK_ACCESS_FS_REFER) != 0)
    {
        fail("best-effort should mask unsupported FS access");
    }
}

static void test_handle_access_net_strict(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(3, LL_ABI_COMPAT_STRICT);
    ll_error_t ret = ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_NET, LANDLOCK_ACCESS_NET_BIND_TCP);
    if (ret != LL_ERROR_UNSUPPORTED_FEATURE)
    {
        fail("strict mode should reject unsupported NET access");
    }
}

static void test_scope_strict(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(5, LL_ABI_COMPAT_STRICT);
    ll_error_t ret = ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_SCOPE,
                                            LANDLOCK_SCOPE_ABSTRACT_UNIX_SOCKET);
    if (ret != LL_ERROR_UNSUPPORTED_FEATURE)
    {
        fail("strict mode should reject unsupported scope");
    }
}

static void test_restrict_self_flags(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);
    ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS,
                           LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR);

    ll_ruleset_t *ruleset = NULL;
    ll_error_t ret = ll_ruleset_create(&attr, 0, &ruleset);
    if (ret != LL_ERROR_OK)
    {
        if (ret == LL_ERROR_UNSUPPORTED_SYSCALL ||
            ret == LL_ERROR_RULESET_CREATE_DISABLED ||
            ret == LL_ERROR_SYSTEM)
        {
            printf("SKIP: kernel does not support Landlock\n");
            return;
        }
        fail("unexpected create ruleset failure in restrict-self test");
        return;
    }

    ret = ll_ruleset_enforce(ruleset, (1U << 31));
    if (ret != LL_ERROR_RESTRICT_FLAGS_INVALID)
    {
        fail("invalid restrict-self flags should be rejected");
    }

    ll_ruleset_close(ruleset);
}

static void test_create_ruleset_best_effort(void)
{
    ll_ruleset_attr_t attr = ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);
    // we must request at least one access right to create a ruleset
    ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS,
                           LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_REFER);
    ll_ruleset_t *ruleset = NULL;
    ll_error_t ret = ll_ruleset_create(&attr, 0, &ruleset);
    if (ret != LL_ERROR_OK)
    {
        if (ret == LL_ERROR_UNSUPPORTED_SYSCALL ||
            ret == LL_ERROR_RULESET_CREATE_DISABLED ||
            ret == LL_ERROR_SYSTEM)
        {
            printf("SKIP: kernel does not support Landlock\n");
            return;
        }
        char msg[128];
        snprintf(msg, sizeof(msg), "unexpected create ruleset failure: %s", ll_error_string(ret));
        fail(msg);
        return;
    }

    ll_ruleset_close(ruleset);
}

static void test_ruleset_enforcement(void)
{
    char template[] = "/tmp/liblandlock-test-XXXXXX";
    char *dir = mkdtemp(template);
    if (!dir)
    {
        fail("failed to create temporary directory");
        return;
    }

    char allowed_path[PATH_MAX];
    snprintf(allowed_path, sizeof(allowed_path), "%s/allowed.txt", dir);

    int fd = open(allowed_path, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (fd < 0)
    {
        fail("failed to create allowed file");
        rmdir(dir);
        return;
    }
    const char *content = "ok\n";
    if (write(fd, content, strlen(content)) < 0)
    {
        fail("failed to write allowed file");
        close(fd);
        unlink(allowed_path);
        rmdir(dir);
        return;
    }
    close(fd);

    pid_t pid = fork();
    if (pid < 0)
    {
        fail("failed to fork test process");
        unlink(allowed_path);
        rmdir(dir);
        return;
    }

    if (pid == 0)
    {
        ll_ruleset_attr_t attr = ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);
        ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS,
                               LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR);

        ll_ruleset_t *ruleset = NULL;
        ll_error_t ret = ll_ruleset_create(&attr, 0, &ruleset);
        if (ret != LL_ERROR_OK)
        {
            if (ret == LL_ERROR_UNSUPPORTED_SYSCALL ||
                ret == LL_ERROR_RULESET_CREATE_DISABLED ||
                ret == LL_ERROR_SYSTEM)
            {
                printf("SKIP: kernel does not support Landlock\n");
                _exit(0);
            }
            _exit(1);
        }

        int dir_fd = open(dir, O_PATH | O_CLOEXEC);
        if (dir_fd < 0)
        {
            _exit(1);
        }

        if (ll_ruleset_add_path_fd(ruleset, dir_fd,
                                   LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR, 0) < 0)
        {
            close(dir_fd);
            _exit(1);
        }
        close(dir_fd);

        ret = ll_ruleset_enforce(ruleset, 0);
        if (ret != LL_ERROR_OK)
        {
            if (ret == LL_ERROR_UNSUPPORTED_SYSCALL ||
                ret == LL_ERROR_RESTRICT_DISABLED ||
                ret == LL_ERROR_RESTRICT_NOT_PERMITTED)
            {
                printf("SKIP: kernel does not support Landlock\n");
                _exit(0);
            }
            _exit(1);
        }

        ll_ruleset_close(ruleset);

        int allowed_fd = open(allowed_path, O_RDONLY);
        if (allowed_fd < 0)
        {
            _exit(1);
        }
        close(allowed_fd);

        int denied_fd = open("/etc/passwd", O_RDONLY);
        if (denied_fd >= 0)
        {
            close(denied_fd);
            _exit(1);
        }
        if (errno != EACCES && errno != EPERM)
        {
            _exit(1);
        }

        _exit(0);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
    {
        fail("failed to wait for child process");
    }
    else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        fail("ruleset enforcement did not behave as expected");
    }

    unlink(allowed_path);
    rmdir(dir);
}

static void test_abi_version_query(void)
{
    ll_abi_t abi = 0;
    const ll_error_t err = ll_get_abi_version(&abi);
    if (err < 0)
    {
        if (err == LL_ERROR_UNSUPPORTED_SYSCALL ||
            err == LL_ERROR_RULESET_CREATE_DISABLED ||
            err == LL_ERROR_SYSTEM)
        {
            printf("SKIP: kernel does not support Landlock\n");
            return;
        }
        char msg[128];
        snprintf(msg, sizeof(msg), "unexpected ABI query failure: %s", ll_error_string(err));
        fail(msg);
    }
    (void)abi;
}

static void test_errata_query(void)
{
    int errata = 0;
    const ll_error_t err = ll_get_errata(&errata);
    if (err < 0)
    {
        if (err == LL_ERROR_UNSUPPORTED_SYSCALL ||
            err == LL_ERROR_RULESET_CREATE_DISABLED ||
            err == LL_ERROR_SYSTEM)
        {
            printf("SKIP: kernel does not support Landlock\n");
            return;
        }
        char msg[128];
        snprintf(msg, sizeof(msg), "unexpected errata query failure: %s", ll_error_string(err));
        fail(msg);
    }
    (void)errata;
}

int main(void)
{
    test_abi_version_query();
    test_errata_query();
    test_create_attr_defaults();
    test_handle_access_fs_strict();
    test_handle_access_fs_best_effort();
    test_handle_access_net_strict();
    test_scope_strict();
    test_restrict_self_flags();
    test_create_ruleset_best_effort();
    test_ruleset_enforcement();

    if (tests_failed == 0)
    {
        printf("OK\n");
        return 0;
    }

    printf("FAILED: %d test(s)\n", tests_failed);
    return 1;
}
