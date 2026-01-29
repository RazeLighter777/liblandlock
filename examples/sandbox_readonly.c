#include "../liblandlock.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int write_file(const char *path, const char *data)
{
    const int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
        return -1;
    }

    const size_t len = strlen(data);
    const ssize_t written = write(fd, data, len);
    close(fd);

    return (written == (ssize_t)len) ? 0 : -1;
}

int main(void)
{
    char template[] = "/tmp/liblandlock-example-XXXXXX";
    char *dir = mkdtemp(template);
    if (!dir)
    {
        perror("mkdtemp");
        return 1;
    }

    char file_path[4096];
    snprintf(file_path, sizeof(file_path), "%s/data.txt", dir);

    if (write_file(file_path, "hello\n") != 0)
    {
        perror("write_file");
        return 1;
    }

    const pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        ll_ruleset_attr_t attr = ll_ruleset_attr_make(LL_ABI_LATEST, LL_ABI_COMPAT_BEST_EFFORT);

        /* Handle both read and write rights, but only allow read on our directory. */
        (void)ll_ruleset_attr_handle(&attr, LL_RULESET_ACCESS_CLASS_FS,
                                     LL_ACCESS_GROUP_FS_READ | LL_ACCESS_GROUP_FS_WRITE);

        ll_ruleset_t *ruleset = NULL;
        ll_sandbox_result_t result = LL_SANDBOX_RESULT_NOT_SANDBOXED;
        ll_error_t err = ll_ruleset_create(&attr, 0, &result, &ruleset);
        if (err != LL_ERROR_OK)
        {
            if (err == LL_ERROR_UNSUPPORTED_SYSCALL ||
                err == LL_ERROR_RULESET_CREATE_DISABLED ||
                err == LL_ERROR_SYSTEM)
            {
                printf("SKIP: Landlock not supported/enabled on this system\n");
                _exit(0);
            }
            fprintf(stderr, "ll_ruleset_create failed: %s (%d)\n", ll_error_string(err), err);
            _exit(1);
        }

        err = ll_ruleset_add_path(ruleset, dir, LL_ACCESS_GROUP_FS_READ, 0);
        if (err != LL_ERROR_OK)
        {
            fprintf(stderr, "ll_ruleset_add_path failed: %s (%d)\n", ll_error_string(err), err);
            ll_ruleset_close(ruleset);
            _exit(1);
        }

        err = ll_ruleset_enforce(ruleset, 0);
        if (err != LL_ERROR_OK)
        {
            if (err == LL_ERROR_UNSUPPORTED_SYSCALL ||
                err == LL_ERROR_RESTRICT_DISABLED ||
                err == LL_ERROR_RESTRICT_NOT_PERMITTED)
            {
                printf("SKIP: Landlock not supported/enabled on this system\n");
                ll_ruleset_close(ruleset);
                _exit(0);
            }
            fprintf(stderr, "ll_ruleset_enforce failed: %s (%d)\n", ll_error_string(err), err);
            ll_ruleset_close(ruleset);
            _exit(1);
        }

        ll_ruleset_close(ruleset);

        /* Read should work */
        const int rd = open(file_path, O_RDONLY);
        if (rd < 0)
        {
            fprintf(stderr, "unexpected: open(O_RDONLY) failed: %s\n", strerror(errno));
            _exit(1);
        }
        close(rd);

        /* Write should be blocked */
        const int wr = open(file_path, O_WRONLY);
        if (wr >= 0)
        {
            fprintf(stderr, "unexpected: open(O_WRONLY) succeeded under read-only sandbox\n");
            close(wr);
            _exit(1);
        }

        printf("OK: write blocked as expected: %s\n", strerror(errno));
        _exit(0);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
    {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return 1;
}
